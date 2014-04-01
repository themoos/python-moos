
import pymoos
import time


#simple example which uses an active queue to handle received messages
#you can send any number of messages to any number of active queues
#here we send binary data just to show we can
comms = pymoos.comms()

def c():
    comms.register('binary_var',0)
    return True
    
def queue_callback(msg):
    if msg.is_binary():
        b = bytearray(msg.binary_data())            
        print 'received ', len(b), 'bytes'
    return True;

def main():
    comms.set_on_connect_callback(c);
    comms.add_active_queue('the_queue',queue_callback)    
    comms.add_message_route_to_active_queue('the_queue','binary_var') 
    comms.run('localhost',9000,'pymoos')
    
    while True:
        time.sleep(1)
        x = bytearray( [0, 3, 0x15, 2, 6, 0xAA] )
        comms.notify_binary('binary_var',bytes(x),pymoos.time());

        
if __name__=="__main__":
    main()

        

