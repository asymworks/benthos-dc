require "formula"

class BenthosDc < Formula
  homepage "https://asymworks.github.com/benthos-dc"
  url "https://github.com/asymworks/benthos-dc/archive/v0.4.2.tar.gz"
  sha256 "6adec9170b5c592bb1fb429acd8b6bb7c994364ea790721bd03182d08d44a7b3"

  head "https://github.com/asymworks/benthos-dc.git"
  
  # Runtime Dependencies
  depends_on "boost"
  #depends_on "libiconv" # Use OS X Library
  #depends_on "libxml2"  # Use OS X Library
  
  # Build Dependencies
  depends_on "cmake" => :build
  depends_on "pkg-config" => :build

  # Build/Install with CMake
  # Options are defaults on OS X
  def install
    build_opts = %W[
      -DBUILD_SHARED=ON
      -DBUILD_STATIC=ON
      -DBUILD_PLUGINS=ON
      -DBUILD_TRANSFER_APP=ON
      -DBUILD_SMARTID=OFF
      -DWITH_IRDA=OFF
      -DWITH_LIBDC=OFF
    ]
    
    system "cmake", ".", *build_opts, *std_cmake_args
    system "make", "install"
  end

  test do
    # Run automated tests of installed plugins
    system "#{bin}/benthos-xfr --test"
  end
end
