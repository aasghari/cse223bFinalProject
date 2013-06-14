import  os
from multiprocessing import Process
def f(name):
    os.system(cmd)
p = Process(target=f, args=(' ',))
cmd= '/home/amellalghamdi/Desktop/cse223bFinalProject/lib/kv_server 1 239.0.2.1 9000 1 2 3 4 '



if __name__ == '__main__':
    p = Process(target=f, args=('',))
    p.start()
    p.join()
