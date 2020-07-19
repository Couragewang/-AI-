LDFLAGS=-ljsoncpp -lcurl -lcrypto 
INCLUDE=-Ispeech
Jarvis:jarvis.cc
	g++ -o $@ $^ $(INCLUDE) $(LDFLAGS) -std=c++11

.PHONY:clean
clean:
	rm -f Jarvis
