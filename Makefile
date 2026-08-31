# If RACK_DIR is not defined when calling make, default to Rack-SDK
RACK_DIR ?= /home/vega/Rack-SDK

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to the deps target in Makefile.
LDFLAGS +=

# Add files to the compilation suite
SOURCES += $(wildcard src/*.cpp)

# Distributables to package into .vcvplugin
DISTRIBUTABLES += res presets $(wildcard LICENSE*) $(wildcard README*)

# Include the Rack-SDK plugin Makefile framework
include $(RACK_DIR)/plugin.mk
