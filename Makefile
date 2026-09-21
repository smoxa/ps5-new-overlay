PS5_PAYLOAD_SDK ?= /opt/ps5-payload-sdk

CC  = $(PS5_PAYLOAD_SDK)/bin/prospero-clang
CXX = $(PS5_PAYLOAD_SDK)/bin/prospero-clang++

CFLAGS   = -O2 -Wall -Wextra -fPIC -fPIE -march=znver2 -Iinclude -I$(PS5_PAYLOAD_SDK)/include -DPS5=1 -D__PS5__=1
CXXFLAGS = $(CFLAGS) -std=c++20
LDFLAGS  = -pie -Wl,--gc-sections -L$(PS5_PAYLOAD_SDK)/lib -lkernel_sys -lSceNotification -lSceLibcInternal -lSceSystemService -lSceSysmodule -lSceUserService

SRCS = src/main.cpp src/monitor.cpp src/overlay_ui.cpp src/notify.cpp src/config.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = dist/ps5_overlay.elf

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p dist
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Built $@"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) dist

.PHONY: all clean
