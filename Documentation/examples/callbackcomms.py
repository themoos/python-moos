import pymoos
import time

#another simple example - here we install a callback which
#fires every time mail arrives. We need to call fetch to
#retrieve it though. We send mail in simple forever loop
#We also subscribe to the same message..

comms = pymoos.comms()

def c():
    return comms.register('simple_var',0)

def m():
    map(lambda msg: msg.trace(), comms.fetch() )
    return True
        
def main():
    
    comms.set_on_connect_callback(c);
    comms.set_on_mail_callback(m);
    comms.run('localhost',9000,'pymoos')

    while True:
        time.sleep(1)
        comms.notify('simple_var','a string',pymoos.time());
    
if __name__=="__main__":
    main()