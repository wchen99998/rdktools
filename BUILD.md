# Build & Test Guide

The following steps reproduce the wheel build and verification process starting from a clean checkout. Commands use macOS defaults; adapt paths or compilers as needed for other platforms.

1. **Create build virtual environment**
   ```bash
   python3 -m venv --clear .venv
   .venv/bin/python -m pip install --upgrade pip
   ```

2. **Install build dependencies**
   ```bash
   .venv/bin/pip install \
     scikit-build-core==0.11.6 \
     nanobind==2.8.0 \
     tensorflow==2.19.0 \
     cmake \
     ninja \
     build \
     auditwheel
   ```

3. **Build the wheel**
   ```bash
   .venv/bin/python -m build --wheel --no-isolation
   ```
   The wheel is written to `dist/rdkit_data_pipeline_tools-<version>-<tag>.whl`.

4. **(macOS only) auditwheel note**
   - `auditwheel` is Linux-only. To check manylinux compliance, copy the wheel to a manylinux container and run:
     ```bash
     auditwheel show dist/<wheel-name>.whl
     ```

5. **Create test environment and install the wheel**
   ```bash
   python3 -m venv --clear .venv_test
   .venv_test/bin/python -m pip install --upgrade pip
   .venv_test/bin/pip install dist/rdkit_data_pipeline_tools-*.whl pytest
   ```

6. **Run automated tests**
   ```bash
   PYTHONPATH='' .venv_test/bin/python -m pytest tests
   ```

7. **Manual verification**
   ```bash
   PYTHONPATH='' .venv_test/bin/python - <<'PY'
   import rdktools
   trace = rdktools.ecfp_reasoning_trace("CCO")
   print(trace.splitlines()[:10])
   print(f"Trace length: {len(trace)} characters")
   PY
   ```
   Expect a multi-line reasoning trace with at least the first few lines describing radius `r0` and per-center chains.

8. **Cleanup (optional)**
   ```bash
   rm -rf .venv .venv_test build
   ```
   Retain `dist/` and `wheelhouse/` as needed.

**Notes**
- The build downloads and compiles Boost and RDKit; the first run can take several minutes.
- Ensure the active Python matches `requires-python (>=3.12)` defined in `pyproject.toml`.
