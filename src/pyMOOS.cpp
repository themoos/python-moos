#if defined(_WIN32)
#define NOMINMAX
#endif

#include <exception>
#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MOOS/libMOOS/Comms/MOOSCommClient.h"
#include "MOOS/libMOOS/Comms/MOOSAsyncCommClient.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl.h>

typedef std::vector<CMOOSMsg> MsgVector;
PYBIND11_MAKE_OPAQUE(MsgVector);
typedef std::vector<MOOS::ClientCommsStatus> CommsStatusVector;
PYBIND11_MAKE_OPAQUE(CommsStatusVector);

namespace py = pybind11;

class pyMOOSException : public std::exception {
public:
    explicit pyMOOSException(const char * m) : message{m} {}
    virtual const char * what() const noexcept override {return message.c_str();}
private:
    std::string message = "";
};

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
        // PyGILState_STATE gstate = PyGILState_Ensure();
        closing_ = true;
        bResult = BASE::Close(true);

        // PyGILState_Release(gstate);
        Py_END_ALLOW_THREADS

        return bResult;
    }


    bool on_connect() {

        bool bResult = false;

        PyGILState_STATE gstate = PyGILState_Ensure();
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
        .def("time", &CMOOSMsg::GetTime, "Return time stamp of message.")
        .def("trace",&CMOOSMsg::Trace, "Print a summary of the message.")

        .def("name", &CMOOSMsg::GetKey, "Return the name of the message.")
        .def("key", &CMOOSMsg::GetKey, "Return the name of the message.")
        .def("is_name", &CMOOSMsg::IsName, "Returns True if name is correct"
                      , py::arg("name"))

        .def("source", &CMOOSMsg::GetSource, "Return the name of the"
                      " process (as registered with the DB) who posted the"
                      " notification.")
        .def("source_aux", &CMOOSMsg::GetSourceAux, "Return the name of the"
                      " process (as registered with the DB) who posted the"
                      " notification auxiliary field.")

        .def("community", &CMOOSMsg::GetCommunity, "Return the name of the "
                      "MOOS community in which the orginator lives")

        .def("is_double", &CMOOSMsg::IsDouble, "Check if data is double.")

        .def("double", &CMOOSMsg::GetDouble, "Return double value of"
                      " message.")
        .def("double_aux",&CMOOSMsg::GetDoubleAux, "Return second double.")

        .def("is_string", &CMOOSMsg::IsString, "Check if data type is "
                      "string.")
        .def("string", &CMOOSMsg::GetString, "Return string value of "
                      "message.")

        .def("is_binary", &CMOOSMsg::IsBinary, "Check if data type is "
                      "binary.")
        .def("binary_data",&CMOOSMsg::GetString, "Return string value (here"
                      " in binary) of the message.")
        .def("binary_data_size",&CMOOSMsg::GetBinaryDataSize, "Return size"
                      " of binary message (0 if not binary type)/")
        .def("mark_as_binary",&CMOOSMsg::MarkAsBinary, "Mark string payload"
                      " as binary.");


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

        .def("register",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&, double)> (&CMOOSCommClient::Register),
                    "Register for notification in changes of named variable.",
                    py::arg("name"), py::arg("interval") = 0)
        .def("register",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&,
                      const std::string&,double)> (&CMOOSCommClient::Register),
                    "Wild card registration",
                    py::arg("var_pattern"), py::arg("app_pattern"),
                    py::arg("interval"))
        .def("is_registered_for",&CMOOSCommClient::IsRegisteredFor,
                    "Return True if we are registered for name variable")

        .def("notify",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&,
                      const std::string&, double)> (&CMOOSCommClient::Notify),
                    "Notify the MOOS community that something has changed.",
                    py::arg("name"), py::arg("value"), py::arg("time") = -1.)
        .def("notify",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&, const std::string&,
                      const std::string&,double)> (&CMOOSCommClient::Notify),
                    "Notify the MOOS community that something has changed.",
                    py::arg("name"), py::arg("value"), py::arg("aux_src"),
                    py::arg("time") = -1.)
        .def("notify",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&,
                      const char *, double)> (&CMOOSCommClient::Notify),
                    "Notify the MOOS community that something has changed.",
                    py::arg("name"), py::arg("value"), py::arg("time") = -1.)
        .def("notify",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&, const char *,
                      const std::string&, double)> (&CMOOSCommClient::Notify),
                    "Notify the MOOS community that something has changed.",
                    py::arg("name"), py::arg("value"), py::arg("aux_src"),
                    py::arg("time") = -1.)
        .def("notify",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&, double,
                      double)> (&CMOOSCommClient::Notify),
                    "Notify the MOOS community that something has changed.",
                    py::arg("name"), py::arg("value"), py::arg("time") = -1.)
        .def("notify",static_cast<bool(CMOOSCommClient::*)
                    (const std::string&, double,
                      const std::string&, double)> (&CMOOSCommClient::Notify),
                    "Notify the MOOS community that something has changed.",
                    py::arg("name"), py::arg("value"), py::arg("aux_src"),
                    py::arg("time") = -1.)

        .def("get_community_name", &CMOOSCommClient::GetCommunityName,
                    "Return name of community the client is attached to.")
        .def("get_moos_name",&CMOOSCommClient::GetMOOSName,
                    "Return the name with which the client registers with the "
                    "MOOSDB.")

        .def("close", &CMOOSCommClient::Close, "Make the client shut down.")

        .def("get_published", &CMOOSCommClient::GetPublished,
                    "Return the list of messages names published.")
        .def("get_registered",&CMOOSCommClient::GetRegistered,
                    "Return the list of messages registered.")
        .def("get_description",&CMOOSCommClient::GetDescription,
                    "Describe this client in a string.")

        .def("is_running", &CMOOSCommClient::IsRunning,
                    "Return True if client is running.")
        .def("is_asynchronous",&CMOOSCommClient::IsAsynchronous,
                    "Return True if this clinet is Asynchronous (so can have "
                    "data pushed to it at any time)")
        .def("is_connected",&CMOOSCommClient::IsConnected,
                    "Return True if this object is connected to the server.")
        .def("wait_until_connected",&CMOOSCommClient::WaitUntilConnected,
                    "Wait for a connection. Return False if not connected "
                    "after n_ms ms.",
                    py::arg("n_ms"))

        .def("get_number_of_unread_messages",
                    &CMOOSCommClient::GetNumberOfUnreadMessages,
                    "How much incoming mail is pending?")

        .def("get_number_of_unsent_messages",
                    &CMOOSCommClient::GetNumberOfUnsentMessages,
                    "How much outgoing mail is pending?")
        .def("get_number_bytes_sent", &CMOOSCommClient::GetNumBytesSent,
                    "Get total number of bytes sent.")
        .def("get_number_bytes_read", &CMOOSCommClient::GetNumBytesReceived,
                    "Get total number of bytes received.")
        .def("get_number_messages_sent", &CMOOSCommClient::GetNumMsgsSent,
                    "Get total number of messages sent.")
        .def("get_number_message_read", &CMOOSCommClient::GetNumMsgsReceived,
                    "Get total number of messages received.")
        .def("set_comms_control_timewarp_scale_factor",
                    &CMOOSCommClient::SetCommsControlTimeWarpScaleFactor,
                    "Set scale factor which determines how to encourage "
                    "messages bunching at high timewarps.",
                    py::arg("scale_factor"))
        .def("get_comms_control_timewarp_scale_factor",
                    &CMOOSCommClient::GetCommsControlTimeWarpScaleFactor,
                    "Get scale factor which determines how to encourage "
                    "messages bunching at high timewarps.")
        .def("do_local_time_correction",
                    &CMOOSCommClient::DoLocalTimeCorrection,
                    "Used to control whether local clock skew (used by "
                    "MOOSTime()) is set via the server at the other end of "
                    "this connection.")

        .def("set_verbose_debug", &CMOOSCommClient::SetVerboseDebug,
                    "Used to control debug printing.")
        .def("set_comms_tick", &CMOOSCommClient::SetCommsTick,
                    "Set the number of ms between loops of the Comms thread "
                    "(not relevent if Asynchronous)",
                    py::arg("comms_tick"))
        .def("set_quiet", &CMOOSCommClient::SetQuiet,
                    "Used to control how verbose the connection process is.")

        .def("enable_comms_status_monitoring",
                    &CMOOSCommClient::EnableCommsStatusMonitoring,
                    "Enable/disable comms status monitoring across the "
                    "community.",
                    py::arg("enable"))
        .def("get_client_comms_status",&CMOOSCommClient::GetClientCommsStatus,
                    "Query the comms status of some other client",
                    py::arg("client"), py::arg("status"))
        .def("get_client_comms_statuses",
                    &CMOOSCommClient::GetClientCommsStatuses,
                    "Get all client statuses",
                    py::arg("statuses"))

    ;
//

    /*********************************************************************
                     Asynchronous comms base class
    *********************************************************************/

    py::class_<MOOS::MOOSAsyncCommClient,CMOOSCommClient>(m, "base_async_comms")
        .def("run",&MOOS::MOOSAsyncCommClient::Run);







    /*********************************************************************/
    /** FINALLY HERE IS THE CLASS WE EXPECT TO BE INSTANTIATED IN PYTHON */;
    // /** FINALLY HERE IS THE CLASS WE EXPECT TO BE INSTANTIATED IN PYTHON */
    // /*********************************************************************/
    //this is the one to use
    py::class_<MOOS::AsyncCommsWrapper, MOOS::MOOSAsyncCommClient>(m, "comms")
        .def(py::init<>())

        .def("run", &MOOS::AsyncCommsWrapper::Run,
                    "Run the MOOSAsyncCommClient.",
                    py::arg("server"), py::arg("port"), py::arg("name"))
        .def("close", &MOOS::AsyncCommsWrapper::Close,
                    "Make the MOOSAsyncCommClient shutdown.",
                    py::arg("nice"))

        .def("fetch", &MOOS::AsyncCommsWrapper::FetchMailAsVector,
                    "Fetch incoming mail as a vector.")
        .def("set_on_connect_callback",
                    &MOOS::AsyncCommsWrapper::SetOnConnectCallback,
                    "Set the user supplied on_connect callback. This callback, "
                    "when set, will be invoked when a connection to the server "
                    "is made. It is a good plan to register for notifications "
                    "of variables in this callback.",
                    py::arg("func"))
        .def("set_on_mail_callback",
                    &MOOS::AsyncCommsWrapper::SetOnMailCallback,
                    "Set the user supplied on_mail callback. If you want rapid "
                    "response, use V10 active callbacks.",
                    py::arg("func"))
        .def("notify_binary", &MOOS::AsyncCommsWrapper::NotifyBinary,
                    "Notify binary data. (specific to pymoos.)",
                    py::arg("name"), py::arg("binary_data"), py::arg("time")=-1)
        .def("add_active_queue", &MOOS::AsyncCommsWrapper::AddActiveQueue,
                    "Register a custom callback for a particular message."
                    "This will be called in it's own thread.",
                    py::arg("queue_name"), py::arg("func"))
        .def("remove_message_route_to_active_queue",
                    &MOOS::AsyncCommsWrapper::RemoveMessageRouteToActiveQueue,
                    "Stop a message routing to active queue.",
                    py::arg("queue_name"), py::arg("msg_name"))
        .def("remove_active_queue", &MOOS::AsyncCommsWrapper::RemoveActiveQueue,
                    "Remove the named active queue.",
                    py::arg("queue_name"))
        .def("has_active_queue", &MOOS::AsyncCommsWrapper::HasActiveQueue,
                    "Does this named active queue exist?",
                    py::arg("queue_name"))
        .def("print_message_to_active_queue_routing",
                    &MOOS::AsyncCommsWrapper::PrintMessageToActiveQueueRouting,
                    "Print all active queues.")
        .def("add_message_route_to_active_queue",
                    static_cast<bool(CMOOSCommClient::*)(const std::string &,
                      const std::string &)>
                      (&MOOS::AsyncCommsWrapper::AddMessageRouteToActiveQueue),
                    "Make a message route to Active Queue (which will call "
                    "a custom callback).",
                    py::arg("queue_name"), py::arg("msg_name"))

        ;


    /*********************************************************************
                    some MOOS Utilities
    *********************************************************************/

    /** here are some global help functions */
    m.def("local_time", &MOOSLocalTime,
              "Return time as double (time since unix in seconds). This "
              "returns the time as reported by the local clock. It will *not* "
              "return time at the Comms Server as time() tries to do.",
              py::arg("apply_timewartping") = true);
    m.def("time", &MOOSTime,
              "Return time as double (time since unix in seconds). This will "
              "aslo apply a skew to this time so that all processes connected "
              "to a MOOSCommsServer (often in the shape of DB) will have a "
              "unified time. Of course, if your process isn't using MOOSComms"
              "at all, this function works just fine and returns the "
              "unadulterated time as you would expect.",
              py::arg("apply_timewartping") = true);
    m.def("is_little_end_in", &IsLittleEndian,
              "Return True if current machine is little end in.");
    m.def("set_moos_timewarp", &SetMOOSTimeWarp,
              "Set the rate at which time is accelerated.",
              py::arg("warp"));
    m.def("get_moos_timewarp", &GetMOOSTimeWarp,
              "Return the current time warp factor.");

    // TODO: double check that it's still needed
    static py::exception<pyMOOSException> ex(m, "pyMOOSException");
    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const pyMOOSException &e) {
            // Set pyMOOSException as the active python error
            // ex(e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
    });


    return m.ptr();
}
