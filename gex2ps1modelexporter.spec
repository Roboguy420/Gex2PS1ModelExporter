Name:           gex2ps1modelexporter
Version:        6
Release:        1%{?dist}
Summary:        Command line program for exporting Gex 2 PS1 models

License:        GPL 
URL:            https://github.com/Roboguy420/Gex2PS1ModelExporter
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:	gcc
BuildRequires:	g++
BuildRequires:	libpng-devel
BuildRequires:	tinyxml2-devel
Requires:       libpng
Requires:	tinyxml2
Requires:	glibc

%description
Command line program for exporting Gex 2 PS1 models

%prep
%setup -q -n Gex2PS1ModelExporter


%build
mkdir "out"
cd "out"
cmake .. -G "Unix Makefiles" --preset x64-release -DCMAKE_INSTALL_PREFIX=%{buildroot}/usr -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DUSE_SHARED_LIBRARIES=1
cd "build/x64-release"
cmake --build .


%install
cd "out/build/x64-release"
echo test246810
cmake --install .
echo test12345678


%files
%license COPYING
/usr/bin/gex2ps1modelexporter
/usr/bin/gex2ps1modelnameslister


%changelog
* Sat Sep 30 2023 Roboguy420
- 
