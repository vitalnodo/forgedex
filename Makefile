EXAMPLES = hello_gui counter_plus_minus fizzbuzz canvas fetch raycaster \
           hello_jni dtmf nativewin shaders themes

.PHONY: help all deps check clean $(EXAMPLES)

#  Bare `make` says what there is rather than building eleven APKs,
#  which is not what anyone wants from a repository they just cloned.
.DEFAULT_GOAL := help

help:
	@echo 'make deps            build fasmg and apksign into deps/  (run once)'
	@echo 'make all             every example'
	@echo 'make <example>       one of them'
	@echo 'make <example>-install'
	@echo 'make <example>-uninstall'
	@echo 'make check           build, then verify dex, manifest and resource checksums'
	@echo 'make clean           remove everything a build writes'
	@echo
	@echo 'examples: $(EXAMPLES)'
	@echo
	@echo 'hello_adb is not built here — it has no APK to make.'
	@echo 'Run it with: sh examples/hello_adb/build.sh'

all: $(EXAMPLES)

deps:
	sh setup_deps.sh

$(EXAMPLES):
	sh examples/$@/build.sh

%-install:
	sh examples/$*/build.sh --install

%-uninstall:
	sh examples/$*/build.sh --uninstall

# hello_adb is not built here: it has no APK to make, and its
# build.sh pushes to a device and runs the class there.

check: all
	sha256sum -c checksums.sha256

#  Everything a build writes, including the pieces assembled from
#  the .asm beside them and the stubs linked against but never
#  shipped.  Leaving those behind meant `clean` left a tree that
#  still differed from a fresh clone.
clean:
	rm -rf examples/*/classes.dex examples/*/app.unsigned.apk \
	       examples/*/app.apk examples/*/lib examples/*/assets \
	       examples/*/lib*.so examples/hello_adb/HelloWorld.apk \
	       examples/*/stub/*.so examples/*/resources.arsc \
	       examples/*/AndroidManifest.xml examples/*/res/layout/*.xml
