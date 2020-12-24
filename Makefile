#
# Makefile
#

.PHONY: tree
tree:
	tree -I "node_modules|dist" -L 3 .

.PHONY: watch
watch:
	while true; do \
		inotifywait -qr -e close_write,modify tests; \
		make; \
	done
