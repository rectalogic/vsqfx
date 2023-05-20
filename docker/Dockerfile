FROM --platform=linux/amd64 ubuntu:jammy as builder

ARG QT_VERSION=6.5.0
ARG QT_PATH=/opt/Qt
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential python3 python3-dev python3-pip python3-venv apt-file
# Install Qt
RUN python3 -m venv /opt/venv && /opt/venv/bin/pip install --no-cache-dir aqtinstall \
    && /opt/venv/bin/aqt install-qt -O "$QT_PATH" linux desktop "$QT_VERSION" gcc_64 \
    && /opt/venv/bin/aqt install-tool -O "$QT_PATH" linux desktop tools_cmake \
    && /opt/venv/bin/aqt install-tool -O "$QT_PATH" linux desktop tools_ninja
# Install missing libs Qt depends on
RUN apt-file update && find "$QT_PATH" -name '*.so' | xargs ldd 2>/dev/null | \
    grep -F '=> not found' | tr '\t' ' ' | cut -d' ' -f2 | sort -u | tee /tmp/not_found_libs.lst ; \
    while read line ; do apt-file find $line | grep '^lib' | head -1; done < /tmp/not_found_libs.lst | cut -d: -f 1 | tee /tmp/to_install_libs.lst


FROM --platform=linux/amd64 ubuntu:jammy

ARG QT_VERSION=6.5.0
ARG QT_PATH=/opt/Qt
ENV DEBIAN_FRONTEND=noninteractive \
    DEBCONF_NONINTERACTIVE_SEEN=true \
    QT_PATH=${QT_PATH} \
    QT_GCC=${QT_PATH}/${QT_VERSION}/gcc_64 \
    PATH=${QT_PATH}/Tools/CMake/bin:${QT_PATH}/Tools/Ninja:${QT_PATH}/${QT_VERSION}/gcc_64/bin:$PATH

RUN apt-get update && apt-get install -y --no-install-recommends \
    git build-essential ninja-build curl python3 python3-dev python3-pip python3-venv locales clang-format
COPY --from=builder /tmp/to_install_libs.lst /tmp/to_install_libs.lst
RUN cat /tmp/to_install_libs.lst | xargs apt install -y --no-install-suggests --no-install-recommends

RUN python3 -m venv /opt/venv && \
    /opt/venv/bin/pip install --no-cache-dir --extra-index-url=https://download.qt.io/official_releases/QtForPython \
    PySide6 shiboken6 shiboken6-generator

COPY --from=builder ${QT_PATH}/ ${QT_PATH}/

RUN locale-gen en_US.UTF-8 && dpkg-reconfigure locales