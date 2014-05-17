import pymoos
import time

# A super simple example:
# a) we make a comms object, and run it
# b) we enter a forever loop in which we both send notifications
#    and check and print mail (with fetch)
# c) the map(lambda,,,) bit simply applies trace() to every message to print
#    a summary of the message.

comms = pymoos.comms()

def on_connect():
    return comms.register('simple_var',0)

def main():
    # my code here
    comms.set_on_connect_callback(on_connect);
    comms.run('localhost',9000,'pymoos')
    
    while True:
        time.sleep(1)
        comms.notify('simple_var','hello world',pymoos.time());
        map(lambda msg: msg.trace(), comms.fetch())

if __name__ == "__main__":
    main()        
