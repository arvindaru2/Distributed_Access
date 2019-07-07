
import os
import string
import random

os.system('cp logs/searchme{,-copy}.txt')

with open('logs/searchme-copy.txt', 'a') as f:
	for _ in range(1000):
		random_str = ''.join([random.choice(string.ascii_lowercase + string.digits) for _ in range(100)])
		f.write(random_str + '\n')

