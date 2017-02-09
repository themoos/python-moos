#include <exception>
#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MOOS/libMOOS/Comms/MOOSCommClient.h"
#include "MOOS/libMOOS/Comms/MOOSAsyncCommClient.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

typedef std::vector<CMOOSMsg> MsgVector;
PYBIND11_MAKE_OPAQUE(MsgVector);
typedef std::vector<MOOS::ClientCommsStatus> CommsStatusVector;
PYBIND11_MAKE_OPAQUE(CommsStatusVector);

namespace py = pybind11;

struct pyMOOSException : std::exception {
    pyMOOSException() {
    }
    ;
    virtual ~pyMOOSException() throw () {
    }
    ;

    pyMOOSException(const std::string & s) :
        s_(s) {
    }
    char const* what() const throw () {
        return s_.c_str();
    }
    std::string s_;
};

void MOOSExceptionTranslator(const pyMOOSException & e) {
    // Use the Python 'C' API to set up an exception object
    PyErr_SetString(PyExc_RuntimeError, e.what());
}

namespace MOOS {

/** this is a class which wraps MOOS::MOOSAsyncCommClient to provide
 * and interface more suitable for python wrapping
 */
class AsyncCommsWrapper : public MOOS::MOOSAsyncCommClient {
private:
    typedef MOOSAsyncCommClient BASE;
public:

    ~AsyncCommsWrapper(){
        Close(true);
    }

    bool Run(const std::string & sServer, int Port, const std::string & sMyName) {
        return BASE::Run(sServer, Port, sMyName, 0);//Comms Tick not used in Async version
    }

    //we can support vectors of objects by not lists so
    //here we have a funtion which copies a list to vector
    MsgVector FetchMailAsVector() {
        MsgVector v;
        MOOSMSG_LIST M;
        if (Fetch(M)) {
            std::copy(M.begin(), M.end(), std::back_inserter(v));
        }

        return v;
    }

    /* python strings can be binary lets make this specific*/
    bool NotifyBinary(const std::string& sKey, const std::string & sData,
                      double dfTime) {
        CMOOSMsg M(MOOS_NOTIFY, sKey, sData.size(), (void *) sData.data(),
                   dfTime);
        return BASE::Post(M);
    }

    static bool on_connect_delegate(void * pParam) {
        MOOS::AsyncCommsWrapper * pMe =
                static_cast<MOOS::AsyncCommsWrapper*> (pParam);
        return pMe->on_connect();
    }
    bool SetOnConnectCallback(py::object func) {
        BASE: SetOnConnectCallBack(on_connect_delegate, this);
        on_connect_object_ = func;
        return true;
    }

    bool Close(bool nice){
        bool bResult = false;

        Py_BEGIN_ALLOW_THREADS
        PyGILState_STATE gstate = PyGILState_Ensure();
        closing_ = true;
        bResult = BASE::Close(true);

        PyGILState_Release(gstate);
        Py_END_ALLOW_THREADS

        return bResult;
    }


    bool on_connect() {

        bool bResult = false;

        // TODO: verify that the following two lines are equivalent.
        PyGILState_STATE gstate = PyGILState_Ensure();
        // py::gil_scoped_acquire acquire;
        try {
            py::object result = on_connect_object_();
            bResult = py::bool_(result);
        } catch (const py::error_already_set& e) {
            PyGILState_Release(gstate);
            throw pyMOOSException(
                                  "OnConnect:: caught an exception thrown in python callback");
        }

        PyGILState_Release(gstate);

        return bResult;

    }

    bool SetOnMailCallback(py::object func) {
        BASE: SetOnMailCallBack(on_mail_delegate, this);
        on_mail_object_ = func;
        return true;
    }

    static bool on_mail_delegate(void * pParam) {
        MOOS::AsyncCommsWrapper * pMe =
                static_cast<MOOS::AsyncCommsWrapper*> (pParam);
        return pMe->on_mail();
    }

    bool on_mail() {
        bool bResult = false;

        PyGILState_STATE gstate = PyGILState_Ensure();
        try {
            if(!closing_){
                py::object result = on_mail_object_();
                bResult = py::bool_(result);
            }
        } catch (const py::error_already_set& e) {
            PyGILState_Release(gstate);
            throw pyMOOSException(
                                  "OnMail:: caught an exception thrown in python callback");
        }

        PyGILState_Release(gstate);

        return bResult;
    }

    static bool active_queue_delegate(CMOOSMsg & M, void* pParam) {
        MeAndQueue * maq = static_cast<MeAndQueue*> (pParam);
        return maq->me_->OnQueue(M, maq->queue_name_);
    }

    bool OnQueue(CMOOSMsg & M, const std::string & sQueueName) {
        std::map<std::string, MeAndQueue*>::iterator q;

        {
            MOOS::ScopedLock L(queue_api_lock_);
            q = active_queue_details_.find(sQueueName);
            if (q == active_queue_details_.end())
                return false;
        }

        bool bResult = false;

        PyGILState_STATE gstate = PyGILState_Ensure();
        try {
            py::object result = q->second->func_(M);
            bResult = py::bool_(result);
        } catch (const py::error_already_set& e) {
            PyGILState_Release(gstate);
            throw pyMOOSException(
                                  "ActiveQueue:: caught an exception thrown in python callback");
        }

        PyGILState_Release(gstate);

        return bResult;

    }

    virtual bool AddActiveQueue(const std::string & sQueueName, py::object func) {

        MOOS::ScopedLock L(queue_api_lock_);

        MeAndQueue* maq = new MeAndQueue;
        maq->me_ = this;
        maq->queue_name_ = sQueueName;
        maq->func_ = func;

        std::cerr << "adding active queue OK\n";

        active_queue_details_[sQueueName] = maq;
        return BASE::AddActiveQueue(sQueueName, active_queue_delegate, maq);

    }


private:

    /** this is a structure which is used to dispatch
     * active queue callbacks
     */
    struct MeAndQueue {
        AsyncCommsWrapper* me_;
        std::string queue_name_;
        py::object  func_;
    };
    std::map<std::string, MeAndQueue*> active_queue_details_;
    CMOOSLock queue_api_lock_;

    /** callback functions (stored) */
    py::object on_connect_object_;
    py::object on_mail_object_;

    /** close connection flag */
    bool closing_;
};
}
;//namesapce


PYBIND11_PLUGIN(pymoos)
{
    py::module m("pymoos", "python wrapping for MOOS.");

    PyEval_InitThreads();


    /*********************************************************************
                     MOOSMsg e class
    *********************************************************************/

    py::class_<CMOOSMsg>(m, "moos_msg")
            .def("time", &CMOOSMsg::GetTime)
            .def("trace",&CMOOSMsg::Trace)

            .def("name", &CMOOSMsg::GetKey)
            .def("key", &CMOOSMsg::GetKey)
            .def("is_name", &CMOOSMsg::IsName)

            .def("source", &CMOOSMsg::GetSource)

            .def("is_double", &CMOOSMsg::IsDouble)

            .def("double", &CMOOSMsg::GetDouble)
            .def("double_aux",&CMOOSMsg::GetDoubleAux)

            .def("is_string", &CMOOSMsg::IsString)
            .def("string", &CMOOSMsg::GetString)

            .def("is_binary", &CMOOSMsg::IsBinary)
            .def("binary_data",&CMOOSMsg::GetString)
            .def("binary_data_size",&CMOOSMsg::GetBinaryDataSize)
            .def("mark_as_binary",&CMOOSMsg::MarkAsBinary);


    /*********************************************************************
                     vector of messages
    *********************************************************************/

    py::bind_vector<MsgVector>(m, "moos_msg_list");


    /*********************************************************************
                     communications status class
    *********************************************************************/

    py::class_<MOOS::ClientCommsStatus>(m,"moos_comms_status")
            .def("appraise",&MOOS::ClientCommsStatus::Appraise)
            .def("print",&MOOS::ClientCommsStatus::Write);


    /*********************************************************************
                     vector of communications status classes
    *********************************************************************/

    py::bind_vector<CommsStatusVector>(m, "moos_comms_status_list");


    /*********************************************************************
                     comms base class
    *********************************************************************/

    py::class_<CMOOSCommObject>(m,"base_comms_object");

    /*********************************************************************
                     synchronous comms base class
    *********************************************************************/
    py::class_<CMOOSCommClient, CMOOSCommObject>(m, "base_sync_comms" )

        .def("register",static_cast<bool(CMOOSCommClient::*)(const std::string&, double)> (&CMOOSCommClient::Register))
        .def("register",static_cast<bool(CMOOSCommClient::*)(const std::string&,const std::string&,double)> (&CMOOSCommClient::Register))
        .def("is_registered_for",&CMOOSCommClient::IsRegisteredFor)

        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const std::string&, double)> (&CMOOSCommClient::Notify), py::arg("sVar"), py::arg("sVal"), py::arg("dfTime") = -1.)
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const std::string&, const std::string&,double)> (&CMOOSCommClient::Notify), py::arg("sVar"), py::arg("sVal"), py::arg("sSrcAux"), py::arg("dfTime") = -1.)
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const char *,double)> (&CMOOSCommClient::Notify))
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const char *,const std::string&,double)> (&CMOOSCommClient::Notify))
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,double,double)> (&CMOOSCommClient::Notify))
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,double,const std::string&,double)> (&CMOOSCommClient::Notify))

        .def("get_community_name", &CMOOSCommClient::GetCommunityName)
        .def("get_moos_name",&CMOOSCommClient::GetMOOSName)

        .def("close", &CMOOSCommClient::Close)

        .def("get_published", &CMOOSCommClient::GetPublished)
        .def("get_registered",&CMOOSCommClient::GetRegistered)
        .def("get_description",&CMOOSCommClient::GetDescription)

        .def("is_running", &CMOOSCommClient::IsRunning)
        .def("is_asynchronous",&CMOOSCommClient::IsAsynchronous)
        .def("is_connected",&CMOOSCommClient::IsConnected)
        .def("wait_until_connected",&CMOOSCommClient::WaitUntilConnected)

        .def("get_number_of_unread_messages",&CMOOSCommClient::GetNumberOfUnreadMessages)

        .def("get_number_of_unsent_messages",&CMOOSCommClient::GetNumberOfUnsentMessages)
        .def("get_number_bytes_sent",&CMOOSCommClient::GetNumBytesSent)
        .def("get_number_bytes_read",&CMOOSCommClient::GetNumBytesReceived)
        .def("get_number_messages_sent",&CMOOSCommClient::GetNumMsgsSent)
        .def("get_number_message_read",&CMOOSCommClient::GetNumMsgsReceived)
        .def("set_comms_control_timewarp_scale_factor",&CMOOSCommClient::SetCommsControlTimeWarpScaleFactor)
        .def("get_comms_control_timewarp_scale_factor", &CMOOSCommClient::GetCommsControlTimeWarpScaleFactor)
        .def("do_local_time_correction",&CMOOSCommClient::DoLocalTimeCorrection)

        .def("set_verbose_debug", &CMOOSCommClient::SetVerboseDebug)
        .def("set_comms_tick",&CMOOSCommClient::SetCommsTick)
        .def("set_quiet",&CMOOSCommClient::SetQuiet)

        .def("enable_comms_status_monitoring",&CMOOSCommClient::EnableCommsStatusMonitoring)
        .def("get_client_comms_status",&CMOOSCommClient::GetClientCommsStatus)
        .def("get_client_comms_statuses",&CMOOSCommClient::GetClientCommsStatuses)

    ;
//

    /*********************************************************************
                     Asynchronous comms base class
    *********************************************************************/

    py::class_<MOOS::MOOSAsyncCommClient, CMOOSCommClient>(m, "base_async_comms")
            .def("run",&MOOS::MOOSAsyncCommClient::Run);







    /*********************************************************************/
    /** FINALLY HERE IS THE CLASS WE EXPECT TO BE INSTANTIATED IN PYTHON */;
    // /** FINALLY HERE IS THE CLASS WE EXPECT TO BE INSTANTIATED IN PYTHON */
    // /*********************************************************************/
    //this is the one to use
    py::class_<MOOS::AsyncCommsWrapper, MOOS::MOOSAsyncCommClient>(m, "comms")
        .def(py::init<>())

        .def("run", &MOOS::AsyncCommsWrapper::Run)
        .def("close", &MOOS::AsyncCommsWrapper::Close)

        .def("fetch",&MOOS::AsyncCommsWrapper::FetchMailAsVector)
        .def("set_on_connect_callback",&MOOS::AsyncCommsWrapper::SetOnConnectCallback)
        .def("set_on_mail_callback",&MOOS::AsyncCommsWrapper::SetOnMailCallback)
        .def("notify_binary",&MOOS::AsyncCommsWrapper::NotifyBinary)
        .def("add_active_queue",&MOOS::AsyncCommsWrapper::AddActiveQueue)
        .def("remove_message_route_to_active_queue",&MOOS::AsyncCommsWrapper::RemoveMessageRouteToActiveQueue)
        .def("remove_active_queue",&MOOS::AsyncCommsWrapper::RemoveActiveQueue)
        .def("has_active_queue",&MOOS::AsyncCommsWrapper::HasActiveQueue)
        .def("print_message_to_active_queue_routing",&MOOS::AsyncCommsWrapper::PrintMessageToActiveQueueRouting)
        .def("add_message_route_to_active_queue",static_cast<bool(CMOOSCommClient::*)(const std::string &,const std::string &)> (&MOOS::AsyncCommsWrapper::AddMessageRouteToActiveQueue))

        ;


    /*********************************************************************
                    some MOOS Utilities
    *********************************************************************/

    /** here are some global help functions */
    m.def("local_time", &MOOSLocalTime, py::arg("bApplyTimeWarping") = true);
    m.def("time", &MOOSTime, py::arg("bApplyTimeWarping") = true);
    m.def("is_little_end_in", &IsLittleEndian);
    m.def("set_moos_timewarp", &SetMOOSTimeWarp);
    m.def("get_moos_timewarp", &GetMOOSTimeWarp);

    // TODO: double check that it's still needed
    // py::register_exception_translator<pyMOOSException>(&MOOSExceptionTranslator);
    // py::register_exception<pyMOOSException>(m, "MOOSExcep");

    return m.ptr();
}
