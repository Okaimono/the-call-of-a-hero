BINARY   := ./build/coah-engine
SHADER_DIR := assets/shaders

# ─────────────────────────────────────────────
#  COMMANDS
#  make reset    — nuke build, reconfigure, rebuild
#  make build    — rebuild (no reconfigure)
#  make shaders  — recompile all shaders
#  make play     — build + run
#  make go       — shaders + build + run  (full launch)
# ─────────────────────────────────────────────

reset:
	rm -rf build
	cmake -B build
	cmake --build build

build:
	cmake --build build

shaders:
	@for f in $(SHADER_DIR)/*.vert; do \
		echo "compiling $$f"; \
		glslc $$f -o $${f%.vert}.spv; \
	done
	@for f in $(SHADER_DIR)/*.frag; do \
		echo "compiling $$f"; \
		glslc $$f -o $${f%.frag}.spv; \
	done

play: build
	$(BINARY)

go: shaders build
	$(BINARY)

.PHONY: reset build shaders play go

vpn:
	protonvpn connect US-FREE#1