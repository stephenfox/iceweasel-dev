#!/usr/bin/make -f

LOCALE := fr_FR.UTF-8
HAS_LOCALE := $(shell locale -a | grep $(LOCALE:UTF-8=utf8))

debian/locales/%:
	mkdir -p debian/locales
	localedef -f $(word 2,$(subst ., ,$(notdir $@))) -i $(word 1,$(subst ., ,$(notdir $@))) $@

ifdef TEST_PATH
TESTS := xpcshell-tests
else
TESTS := check xpcshell-tests reftest crashtest jstestbrowser
endif

override_dh_auto_test: $(TESTS)

debian/reftest-app/distribution: $(CURDIR)/build-xulrunner/dist/bin/distribution
	ln -s $< $@

debian/reftest-app/stub: $(CURDIR)/build-xulrunner/dist/bin/xulrunner-stub
	ln -s $< $@

ifndef HAS_LOCALE
xpcshell-tests: export LOCPATH = $(CURDIR)/debian/locales
endif
xpcshell-tests: export LC_ALL=$(LOCALE)
xpcshell-tests: export NSPR_LOG_MODULES=all:5
reftest crashtest jstestbrowser: debian/reftest-app/stub debian/reftest-app/distribution
reftest crashtest jstestbrowser: export EXTRA_TEST_ARGS += --appname=$(CURDIR)/debian/reftest-app/stub
reftest crashtest jstestbrowser: export GRE_HOME = $(CURDIR)/build-xulrunner/dist/bin
reftest crashtest jstestbrowser: XVFB_RUN = xvfb-run -s "-screen 0 1024x768x24"

$(TESTS):
	GNOME22_USER_DIR="$(CURDIR)/dist/.gnome2" \
	HOME="$(CURDIR)/dist" \
	$(XVFB_RUN) $(MAKE) -C build-xulrunner $@ 2>&1 | sed -u 's/^/$@> /'

xpcshell-tests: xpcshell-tests-skip $(if $(HAS_LOCALE),,debian/locales/$(LOCALE))

xpcshell-tests-skip:
# APNG is not supported
	rm -f build-xulrunner/_tests/xpcshell/test_libpr0n/unit/test_encoder_apng.js
# Image tests fail because of the use of system library which don't give the same result
	rm -f build-xulrunner/_tests/xpcshell/test_libpr0n/unit/test_encoder_png.js \
	      build-xulrunner/_tests/xpcshell/test_libpr0n/unit/test_imgtools.js \
	      build-xulrunner/_tests/xpcshell/test_places/unit/test_favicons.js
# This one fails because it relies on a sqlite bug that is fixed in the system one
# See http://hg.mozilla.org/mozilla-central/raw-rev/1192461c259d
	rm -f build-xulrunner/_tests/xpcshell/test_storage/unit/test_storage_combined_sharing.js
# This one fails because it supposes some kind of preexisting gnome/mailcap configuration
	rm -f build-xulrunner/_tests/xpcshell/test_uriloader_exthandler/unit/test_handlerService.js
# This one fails because of the patch from bz#527458. Anyways, if it failed, a lot of other tests would fail, too, so it is safe to ignore it
	rm -f build-xulrunner/_tests/xpcshell/test_testing_xpcshell_example/unit/test_load_httpd_js.js

check: check-skip

check-skip:
# This one fails because it only works in an american time zone. bz#515254
	rm -f js/src/trace-test/tests/sunspider/check-date-format-tofte.js

override_dh_auto_clean::
	rm -rf debian/locales debian/reftest-app/stub debian/reftest-app/distribution

.PHONY: test $(TESTS) xpcshell-tests-skip
