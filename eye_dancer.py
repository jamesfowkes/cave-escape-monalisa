import requests
import time

for i in range(60):
	requests.get("http://192.168.0.134/move/{}".format(i*6))
	time.sleep(0.1)

requests.get("http://192.168.0.134/move/0")

time.sleep(0.2)

requests.get("http://192.168.0.134/reset")

requests.get("http://192.168.0.134/blink/3/250/300")

