import pymoos
import time

#another simple example - here we install a callback which
#fires every time mail arrives. We need to call fetch to
#retrieve it though. We send mail in simple forever loop
#We also subscribe to the same message..

#This version demonstrates setting and using a timewarp that allows speeding up
#simulations with MOOS

comms = pymoos.comms()

def c():
    print("\t|-Time Warp @ {0:.1f} \n".format(pymoos.get_moos_timewarp()))
    print("\t|-Time Warp delay @ {0:.1f} ms \n".format( \
        comms.get_comms_control_timewarp_scale_factor()*pymoos.get_moos_timewarp()))
    return comms.register('simple_var',0)

def m():
    map(lambda msg: msg.trace(), comms.fetch() )
    return True
        
def main():
    
    comms.set_on_connect_callback(c)
    comms.set_on_mail_callback(m)
    pymoos.set_moos_timewarp(10)
    comms.set_comms_control_timewarp_scale_factor(0.4)
    comms.run('localhost',9000,'pymoos')

    while True:
        time.sleep(1.0)
        comms.notify('simple_var','a string',pymoos.time())
    
if __name__=="__main__":
    main()