include Makefile.common

# Build all libraries

all:
	$(MAKE) all -C Devices/MaxMoisture
	$(MAKE) all -C Devices/MaxSniffer

clean:
	$(MAKE) clean -C Devices/MaxMoisture
	$(MAKE) clean -C Devices/MaxSniffer
	