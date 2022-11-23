.PHONY: lib, pybind, clean, format, all, config

all: lib

config: 
	@if not exist build mkdir build
	@cd build && cmake ..

lib:
	@cd build && $(MAKE)

format:
	python3 -m black .
	clang-format -i src/*.cc src/*.cu

clean:
	rm -rf build python/needle/backend_ndarray/ndarray_backend*.so
