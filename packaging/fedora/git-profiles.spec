Name:           git-profiles
Version:        0.0.3
Release:        1%{?dist}
Summary:        Organize your git repositories by SSH profile

License:        MIT
URL:            https://github.com/hernique33comiitei/git-profiles
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  extra-cmake-modules
BuildRequires:  gettext
BuildRequires:  desktop-file-utils
BuildRequires:  appstream
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  kf6-kirigami-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-kcoreaddons-devel

# Runtime libraries (Qt6/KF6) are picked up automatically from the ELF, but the
# QML modules are dlopened, so require them explicitly.
Requires:       qt6-qtdeclarative
Requires:       qt6-qtquickcontrols2
Requires:       kf6-kirigami
Requires:       kf6-ki18n
Requires:       kf6-kcoreaddons
# App-managed dependencies (not git/gh/gt/glab, which the app only checks for):
Requires:       direnv >= 2.37.1
Requires:       libsecret
# git is required at runtime but the app detects and guides, so keep it weak:
Recommends:     git

%description
Git Profiles manages SSH/Git identities straight from ~/.ssh/config. It switches
repository remotes in bulk (with a dry-run preview) and configures the provider
CLI (gh / gt / glab) per directory via direnv, keeping tokens in the keyring.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -GNinja
%cmake_build

%install
%cmake_install
# The gettext textdomain is "gitprofiles" (no hyphen), unlike the package name.
%find_lang gitprofiles

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/io.github.hernique33comiitei.GitProfiles.desktop
appstreamcli validate --no-net %{buildroot}%{_metainfodir}/io.github.hernique33comiitei.GitProfiles.metainfo.xml || :

%files -f gitprofiles.lang
%license LICENSE
%doc README.md
%{_bindir}/gitprofiles
%{_datadir}/applications/io.github.hernique33comiitei.GitProfiles.desktop
%{_metainfodir}/io.github.hernique33comiitei.GitProfiles.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/gitprofiles.svg
%{_datadir}/icons/hicolor/scalable/apps/gitprofiles-symbolic.svg

%changelog
* Tue Nov 17 2026 Henrique Oliveira <henriik.dev@gmail.com> - 0.0.3-1
- Test connection now checks the details you are editing, even before you save
- Show a placeholder on the Profiles page when there are no profiles yet

* Tue Nov 17 2026 Henrique Oliveira <henriik.dev@gmail.com> - 0.0.2-1
- Use generic example placeholders instead of real hostnames
- Add dark-theme screenshots to the app metadata
- Rewrite the README with RPM/DEB install instructions
- Single-source the version and automate RPM/DEB release builds

* Mon Nov 16 2026 Henrique Oliveira <henriik.dev@gmail.com> - 0.0.1-1
- Initial package
