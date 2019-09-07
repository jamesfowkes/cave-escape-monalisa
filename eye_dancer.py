import requests
import time
import sys

ip = sys.argv[1]

requests.get("http://{}/open".format(ip))

time.sleep(2)

for i in range(60):
	requests.get("http://{}/move/{}".format(ip, i*6))
	time.sleep(0.1)

requests.get("http://{}/move/0".format(ip))

time.sleep(0.2)

requests.get("http://{}/reset".format(ip))

requests.get("http://{}/blink/3/250/300".format(ip))

