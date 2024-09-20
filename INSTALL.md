# Installation Guide

## Prerequisites

Before installing the `hydrosheds` Python package, ensure you have the following
dependencies installed:

1. **Python**: Make sure you have Python installed. You can download it from
   [python.org](https://www.python.org/).
2. **pip**: Ensure you have pip installed. You can install it using:
    ```sh
    python -m ensurepip --upgrade
    ```

3. **C++ Libraries**:
    - **Eigen3**: Install Eigen3 from
        [eigen.tuxfamily.org](https://eigen.tuxfamily.org/dox/GettingStarted.html).
    - **pybind11**: Install pybind11 using pip:
        ```sh
        pip install pybind11
        ```
    - **GDAL**: Install GDAL. On Ubuntu, you can use:
        ```sh
        sudo apt-get install gdal-bin libgdal-dev
        ```
      On macOS, you can use:
        ```sh
        brew install gdal
        ```

## Installation

Once all prerequisites are installed, you can install the `hydrosheds` package
using pip.

First, clone the repository from GitHub and then install the package locally
using pip, as it is not yet deployed on PyPI:

```sh
git clone https://github.com/fbriol/hydrosheds.git
cd hydrosheds
pip install .
```

## Verifying the Installation

To verify that the installation was successful, you can run:

```sh
python -c "import hydrosheds; print('hydrosheds installed successfully')"
```

If you see the message `hydrosheds installed successfully`, the installation was
successful.
