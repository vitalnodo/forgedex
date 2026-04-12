EXAMPLES = hello_gui counter_button counter_plus_minus hello_jni

.PHONY: all deps check clean $(EXAMPLES)

all: $(EXAMPLES)

deps:
	sh setup_deps.sh

$(EXAMPLES):
	sh examples/$@/build.sh

%-install:
	sh examples/$*/build.sh --install

%-uninstall:
	sh examples/$*/build.sh --uninstall

check: all
	sha256sum -c checksums.sha256

clean:
	rm -rf examples/*/classes.dex examples/*/app.unsigned.apk examples/*/app.apk examples/*/lib
