FROM alpine:latest

ENV USER=avrdev
ENV UID=1000
ENV GID=$UID

# Create the user
RUN addgroup -g $GID "$USER"
RUN adduser --disabled-password --gecos "" --ingroup "$USER" --uid "$UID" "$USER"

# Install required packages
RUN apk add --no-cache build-base autoconf automake pkgconf libtool libusb libusb-dev libftdi1 libftdi1-dev git mc meson gettext-dev avrdude linux-headers texinfo make gcc-avr avr-libc cmake ca-certificates

USER $USER