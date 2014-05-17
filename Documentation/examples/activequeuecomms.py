import pymoos
import time


#simple example which uses an active queue to handle received messages
#you can send any number of messages to any number of active queues
comms = pymoos.comms()

def c():
    comms.register('simple_var',0)
    return True
    
def queue_callback(msg):
    msg.trace();    
    return True;

def main():
    
    comms.set_on_connect_callback(c);
    comms.add_active_queue('the_queue',queue_callback)    
    comms.add_message_route_to_active_queue('the_queue','simple_var') 
    comms.run('localhost',9000,'pymoos')
    
    while True:
        time.sleep(1)
        comms.notify('simple_var','hello world',pymoos.time());
        
if __name__=="__main__":
        main()

        