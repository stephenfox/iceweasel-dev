# Generic rules to help download sources from ftp.mozilla.org.
# Define the following variables before including this file:
# PRODUCT - product codename (e.g. browser)
# PRODUCT_NAME - name of the product (e.g. firefox)

# The VERSION_FILTER transforms upstream version patterns to versions
# used in debian/changelog. Versions are to be transformed as follows:
# 4.0      -> 4.0
# 4.0pre   -> 4.0~pre
# 4.0b5    -> 4.0~b5
# 4.0b5pre -> 4.0~b5~pre
# That should ensure the proper ordering
VERSION_FILTER := sed 's/\([0-9]\)\([a-z]\)/\1~\2/g'
UPSTREAM_VERSION := $(shell cat $(PRODUCT)/config/version.txt)
GRE_MILESTONE := $(shell config/milestone.pl --topsrcdir .| $(VERSION_FILTER))

# Construct GRE_VERSION from the first two digits in GRE_MILESTONE
GRE_VERSION := $(subst ~, ,$(subst ., ,$(GRE_MILESTONE)))
export JS_SO_VERSION := $(firstword $(GRE_VERSION))d
export GRE_VERSION := $(firstword $(GRE_VERSION)).$(word 2,$(GRE_VERSION))

# Last version in debian/changelog
DEBIAN_SRC_VERSION := $(shell dpkg-parsechangelog | sed -n 's/^\(Source\|Version\): *// p')
DEBIAN_SOURCE := $(firstword $(DEBIAN_SRC_VERSION))
DEBIAN_VERSION := $(word 2, $(DEBIAN_SRC_VERSION))
# Debian part of the above version (anything after the last dash)
DEBIAN_RELEASE := $(lastword $(subst -, ,$(DEBIAN_VERSION)))
# Upstream part of the debian/changelog version (anything before the last dash)
UPSTREAM_RELEASE := $(DEBIAN_VERSION:%-$(DEBIAN_RELEASE)=%)
# Aurora builds have the build id in the upstream part of the debian/changelog version
export MOZ_BUILD_DATE := $(word 2,$(subst +, ,$(UPSTREAM_RELEASE)))
UPSTREAM_RELEASE := $(firstword $(subst +, ,$(UPSTREAM_RELEASE)))

# Check if the version in debian/changelog matches actual upstream version
# as VERSION_FILTER transforms it.
FILTERED_UPSTREAM_VERSION := $(shell echo $(UPSTREAM_VERSION) | $(VERSION_FILTER))
ifneq ($(FILTERED_UPSTREAM_VERSION),$(firstword $(subst ~b, ,$(UPSTREAM_RELEASE))))
$(error Upstream version in debian/changelog ($(UPSTREAM_RELEASE)) does not match actual upstream version ($(FILTERED_UPSTREAM_VERSION)))
endif

SOURCE_TARBALL = $(DEBIAN_SOURCE)_$(UPSTREAM_RELEASE)$(SOURCE_BUILD_DATE:%=+%).orig.tar.bz2
SOURCE_TARBALL_LOCATION = ..

SOURCE_VERSION = $(subst ~,,$(UPSTREAM_RELEASE))

# Find the right channel corresponding to the version number
ifneq (,$(filter suite mail calendar,$(PRODUCT)))
REPO_PREFIX = comm
else
REPO_PREFIX = mozilla
endif
ifneq (,$(filter $(GRE_MILESTONE)~b%, $(UPSTREAM_RELEASE)))
# Betas are under releases/
SOURCE_TYPE := releases
else
ifneq (,$(filter %~a2, $(UPSTREAM_RELEASE)))
# Aurora
SOURCE_TYPE := nightly
SOURCE_CHANNEL := $(REPO_PREFIX)-aurora
else
ifneq (,$(filter %~a1, $(UPSTREAM_RELEASE)))
# Nightly
SOURCE_TYPE := nightly
SOURCE_CHANNEL := $(REPO_PREFIX)-central
else
# Release
SOURCE_TYPE := releases
endif
endif
endif

BASE_URL = ftp://ftp.mozilla.org/pub/mozilla.org/$(PRODUCT_NAME)/$(SOURCE_TYPE)

ifeq ($(SOURCE_TYPE),releases)
SOURCE_URL = $(BASE_URL)/$(SOURCE_VERSION)/source/$(PRODUCT_NAME)-$(SOURCE_VERSION).source.tar.bz2
else
ifeq ($(SOURCE_TYPE),nightly)
LATEST_NIGHTLY = $(if $(_LATEST_NIGHTLY),,$(eval _LATEST_NIGHTLY := $(shell $(PYTHON) debian/latest_nightly.py $(BASE_URL)/latest-$(SOURCE_CHANNEL))))$(_LATEST_NIGHTLY)
SOURCE_BUILD_DATE = $(firstword $(LATEST_NIGHTLY))
SOURCE_URL = $(subst /rev/,/archive/,$(word 2, $(LATEST_NIGHTLY))).tar.bz2
endif
endif

ifneq (,$(filter download,$(MAKECMDGOALS)))
download: $(SOURCE_TARBALL_LOCATION)/$(SOURCE_TARBALL)

$(SOURCE_TARBALL_LOCATION)/$(SOURCE_TARBALL): debian/source.filter
	$(PYTHON) debian/repack.py -o $@ $(SOURCE_URL)
endif
.PHONY: download
