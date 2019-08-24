require "formula"

class BenthosDc < Formula
  homepage "https://asymworks.github.com/benthos-dc"
  url "https://github.com/asymworks/benthos-dc/archive/v0.4.2.tar.gz"
  sha1 "fcb5a46951cb4ead3a1191fb2193e4ca132fc07e"

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
