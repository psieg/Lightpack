import socket, time, imaplib, re, sys

class lightpack:

#	host = '127.0.0.1'    # The remote host
#	port = 3636              # The same port as used by the server
#	apikey = 'key'		  # Secure API key which generates by Lightpack software on Dev tab
#	ledMap = [1,2,3,4,5,6,7,8,9,10] 	#mapped LEDs

	def __init__(self, _host, _port, _ledMap, _apikey = None):
		self.host = _host
		self.port = _port
		self.ledMap = _ledMap
		self.apikey = _apikey

	def __readResult(self):	# Return last-command API answer  (call in every local method)
		total_data=[]
		data = self.connection.recv(8192)
		total_data.append(data)
		return b"".join(total_data)

	def getProfiles(self):
		self.connection.send(b"getprofiles\n")
		profiles = self.__readResult().decode()
		return profiles.split(':')[1].rstrip(';\n\r').split(';')

	def getProfile(self):
		self.connection.send(b"getprofile\n")
		profile = self.__readResult().decode()
		profile = profile.split(':')[1]
		return profile

	def getStatus(self):
		self.connection.send(b"getstatus\n")
		status = self.__readResult().decode()
		status = status.split(':')[1]
		return status

	def getCountLeds(self):
		self.connection.send(b"getcountleds\n")
		count = self.__readResult().decode()
		count = count.split(':')[1]
		return count

	def getAPIStatus(self):
		self.connection.send(b"getstatusapi\n")
		status = self.__readResult().decode()
		status = status.split(':')[1]
		return status

	def connect (self):
		try: 	#Try to connect to the server API
			self.connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			self.connection.connect((self.host, self.port))
			self.__readResult()
			if self.apikey is not None:
				cmd = 'apikey:' + self.apikey + '\n'
				self.connection.send(bytes(cmd, 'utf-8'))
				self.__readResult()
			return 0
		except:
			print('Lightpack API server is missing')
			return -1

	def setColor(self, n, r, g, b): 	# Set color to the define LED
		cmd = 'setcolor:{0}-{1},{2},{3}\n'.format(self.ledMap[n-1], r, g, b)
		self.connection.send(str.encode(cmd))
		return self.__readResult().decode()

	def setColorToAll(self, r, g, b): 	# Set one color to all LEDs
		cmdstr = ''
		for i in self.ledMap:
			cmdstr = str(cmdstr) + str(i) + '-{0},{1},{2};'.format(r,g,b)
		cmd = 'setcolor:%s\n' % cmdstr
		#print("cmd: " + cmd, self.ledMap);
		self.connection.send(str.encode(cmd))
		return self.__readResult().decode()

	def setGamma(self, g):
		cmd = 'setgamma:{0}\n'.format(g)
		self.connection.send(str.encode(cmd))
		return self.__readResult()

	def setSmooth(self, s):
		cmd = 'setsmooth:{0}\n'.format(s)
		self.connection.send(str.encode(cmd))
		return self.__readResult()

	def setBrightness(self, s):
		cmd = 'setbrightness:{0}\n'.format(s)
		self.connection.send(str.encode(cmd))
		return self.__readResult()

	def setProfile(self, p):
		cmd = 'setprofile:%s\n' % p
		self.connection.send(str.encode(cmd))
		return self.__readResult()

	def lock(self):
		self.connection.send(b"lock\n")
		return self.__readResult()

	def unlock(self):
		self.connection.send(b"unlock\n")
		return self.__readResult()

	def turnOn(self):
		self.connection.send(b"setstatus:on\n")
		return self.__readResult().decode()

	def turnOff(self):
		self.connection.send(b"setstatus:off\n")
		return self.__readResult().decode()

	def disconnect(self):
		self.unlock()
		self.connection.close()
