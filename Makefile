PS5_HOST ?= ps5
PS5_PORT ?= 9021

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error PS5_PAYLOAD_SDK is undefined)
endif

ELF := ps5-pup-decrypt.elf

CFLAGS := -std=c++11 -Wall -Werror -g -I./include

all: $(ELF)

$(ELF): source/checkheaders.c source/decrypt.c source/decryptio.c source/encryptsrv.c source/main.c
	$(CXX) $(CFLAGS) -o $@ $^

clean:
	rm -f $(ELF)
