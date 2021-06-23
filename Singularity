################################################################################
# Basic bootstrap definition to build Ubuntu container from Docker container
################################################################################

Bootstrap:docker
From:ubuntu:18.04

%labels
Maintainer Emily Dolson
Version 2.0.0

################################################################################
# Copy any necessary files into the container
################################################################################
%files
. /opt/force-cover

%post
################################################################################
# Install additional packages
################################################################################
apt-get update
apt-get install -y software-properties-common
apt install -y libclang-dev llvm-dev clang

################################################################################
# Run the user's login shell, or a user specified command
################################################################################
%runscript
echo "Nothing here but us birds!"
