LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..
SRC_DIR := $(CORE_DIR)/src
LIBRETRO_DIR := $(SRC_DIR)/deps/libretro-common

# Emulation core sources
CORE_SOURCES := \
	libretro.c \
	intv.c \
	memory.c \
	cp1610.c \
	cart.c \
	controller.c \
	osd.c \
	ivoice.c \
	psg.c \
	stic.c \
	stb_image_impl.c

# libretro-common sources
LIBRETRO_SOURCES := \
	file_path.c \
	compat_posix_string.c \
	compat_snprintf.c \
	compat_strl.c \
	compat_strcasestr.c \
	fopen_utf8.c \
	encoding_utf.c \
	stdstring.c \
	rtime.c

# Convert to full paths
LOCAL_SRC_FILES := $(addprefix $(SRC_DIR)/,$(CORE_SOURCES))
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/file/file_path.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/compat/compat_posix_string.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/compat/compat_snprintf.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/compat/compat_strl.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/compat/compat_strcasestr.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/compat/fopen_utf8.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/encodings/encoding_utf.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/string/stdstring.c
LOCAL_SRC_FILES += $(LIBRETRO_DIR)/time/rtime.c

include $(CLEAR_VARS)
LOCAL_MODULE            := retro_freeintvds
LOCAL_SRC_FILES         := $(LOCAL_SRC_FILES)
LOCAL_C_INCLUDES        := $(SRC_DIR) $(LIBRETRO_DIR)/include
LOCAL_CFLAGS            := -DANDROID -D__LIBRETRO__ -DHAVE_STRINGS_H -DRIGHTSHIFT_IS_SAR -DFREEINTV_DS -DDEBUG_ANDROID
LOCAL_LDFLAGS           := -Wl,-version-script=$(CORE_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
