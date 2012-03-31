import subprocess
import time

out = open('output', 'w')

process = subprocess.Popen(['red_db'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

process.stdin.write("c: INSERT into users values ('hristo','asenov',1)\n")
#process.stdin.write("a: uname = hristopass = asenovactivated = 1\n")

process.stdin.write("c: INSERT into users values ('george','doychev',0)\n")
#process.stdin.write("a: uname = georgepass = doychevactivated = 0\n")
process.stdin.write("c: INSERT into users values ('iana','asenova',0)\n")
#process.stdin.write("a: uname = ianapass = asenovaactivated = 0\n")
process.stdin.write("SELECT * from users\n")

stdout =  process.communicate()[0]
print stdout

time.sleep(20)
#process.stdin.write("INSERT into users values ('george','doichev',0)\n")
#process.stdin.write("SELECT * from users\n")


process.kill()
