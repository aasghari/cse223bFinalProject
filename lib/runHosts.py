import subprocess
child1 = subprocess.Popen("xterm -e python h1.py", shell=True)
child2 = subprocess.Popen("xterm -e python h2.py", shell=True)
child3 = subprocess.Popen("xterm -e python h3.py", shell=True)
child4 = subprocess.Popen("xterm -e python h4.py", shell=True)
