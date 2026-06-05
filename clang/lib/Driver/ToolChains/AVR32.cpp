//===--- AVR32.cpp - AVR32 ToolChain Implementations ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32.h"
#include "Gnu.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/InputInfo.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <optional>

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;
using namespace llvm::opt;

namespace {

StringRef getAVR32PartName(const ArgList &Args) {
  if (const Arg *A = Args.getLastArg(options::OPT_mpart_EQ))
    return A->getValue();
  if (const Arg *A = Args.getLastArg(options::OPT_mmcu_EQ))
    return A->getValue();
  return "";
}

std::string getAVR32Emulation(const ArgList &Args) {
  StringRef Part = getAVR32PartName(Args);
  if (Part == "uc3a3revd")
    return "avr32elf_uc3a3256s";
  if (!Part.empty())
    return ("avr32elf_" + Part).str();

  StringRef CPU = Args.getLastArgValue(options::OPT_mcpu_EQ);
  if (!CPU.empty() && CPU != "avr32")
    return ("avr32elf_" + CPU).str();
  return "";
}

StringRef getAVR32Multilib(const ArgList &Args) {
  StringRef Part = getAVR32PartName(Args);
  if (Part == "uc3a3256" || Part == "uc3a3256s" || Part == "uc3a3revd")
    return "ucr2";

  StringRef Arch = Args.getLastArgValue(options::OPT_march_EQ);
  if (Arch == "ap" || Arch == "ucr1" || Arch == "ucr2" || Arch == "ucr2nomul" ||
      Arch == "ucr3" || Arch == "ucr3fp")
    return Arch;
  return "";
}

void addAVR32LibPath(const Driver &D, ToolChain::path_list &Paths,
                     StringRef Base, StringRef Multilib) {
  SmallString<128> Path(Base);
  llvm::sys::path::append(Path, "lib");
  if (!Multilib.empty())
    llvm::sys::path::append(Path, Multilib);
  addPathIfExists(D, Path, Paths);
}

void addAVR32GCCLibPathRoot(const Driver &D, ToolChain::path_list &Paths,
                            SmallString<128> GCCRoot, StringRef Multilib) {
  if (!llvm::sys::fs::is_directory(GCCRoot))
    return;

  std::error_code EC;
  for (llvm::sys::fs::directory_iterator I(GCCRoot, EC), E; I != E && !EC;
       I.increment(EC)) {
    if (!llvm::sys::fs::is_directory(I->path()))
      continue;

    SmallString<128> VersionPath(I->path());
    if (!Multilib.empty()) {
      SmallString<128> MultilibPath(VersionPath);
      llvm::sys::path::append(MultilibPath, Multilib);
      addPathIfExists(D, MultilibPath, Paths);
    }
    addPathIfExists(D, VersionPath, Paths);
  }
}

void addAVR32GCCLibPaths(const Driver &D, ToolChain::path_list &Paths,
                         StringRef SysRoot, StringRef Multilib) {
  SmallString<128> Prefix(SysRoot);
  llvm::sys::path::append(Prefix, "..");
  llvm::sys::path::remove_dots(Prefix, /*remove_dot_dot=*/true);

  SmallString<128> SiblingGCC(Prefix);
  llvm::sys::path::append(SiblingGCC, "lib", "gcc", "avr32");
  addAVR32GCCLibPathRoot(D, Paths, SiblingGCC, Multilib);

  SmallString<128> SysRootGCC(SysRoot);
  llvm::sys::path::append(SysRootGCC, "lib", "gcc", "avr32");
  addAVR32GCCLibPathRoot(D, Paths, SysRootGCC, Multilib);
}

std::optional<std::string> getAVR32LinkerScript(StringRef SysRoot,
                                                StringRef Emulation) {
  if (SysRoot.empty() || Emulation.empty())
    return std::nullopt;

  SmallString<128> Path(SysRoot);
  llvm::sys::path::append(Path, "lib", "ldscripts", Emulation + ".x");
  if (llvm::sys::fs::exists(Path))
    return std::string(Path);
  return std::nullopt;
}

} // end anonymous namespace

AVR32ToolChain::AVR32ToolChain(const Driver &D, const llvm::Triple &Triple,
                               const ArgList &Args)
    : Generic_ELF(D, Triple, Args) {
  GCCInstallation.init(Triple, Args);

  StringRef Multilib = getAVR32Multilib(Args);
  if (GCCInstallation.isValid()) {
    SmallString<128> GCCBinPath(GCCInstallation.getParentLibPath());
    llvm::sys::path::append(GCCBinPath, "..", "bin");
    addPathIfExists(D, GCCBinPath, getProgramPaths());

    SmallString<128> GCCLibPath(GCCInstallation.getInstallPath());
    if (!Multilib.empty())
      llvm::sys::path::append(GCCLibPath, Multilib);
    addPathIfExists(D, GCCLibPath, getFilePaths());
    addPathIfExists(D, GCCInstallation.getInstallPath(), getFilePaths());
  }

  std::string SysRoot = computeSysRoot();
  addAVR32GCCLibPaths(D, getFilePaths(), SysRoot, Multilib);
  addAVR32LibPath(D, getFilePaths(), SysRoot, Multilib);
  addAVR32LibPath(D, getFilePaths(), SysRoot, "");
}

std::string AVR32ToolChain::computeSysRoot() const {
  if (!getDriver().SysRoot.empty())
    return getDriver().SysRoot;

  if (GCCInstallation.isValid()) {
    SmallString<128> Dir(GCCInstallation.getInstallPath());
    llvm::sys::path::append(Dir, "..", "..", "..");
    llvm::sys::path::append(Dir, "..", "avr32");
    llvm::sys::path::remove_dots(Dir, /*remove_dot_dot=*/true);
    return std::string(Dir);
  }

  SmallString<128> Dir(getDriver().Dir);
  llvm::sys::path::append(Dir, "..", "avr32");
  return std::string(Dir);
}

void AVR32ToolChain::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                               ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdinc) ||
      DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;

  SmallString<128> IncludeDir(computeSysRoot());
  llvm::sys::path::append(IncludeDir, "include");
  addSystemInclude(DriverArgs, CC1Args, IncludeDir.str());
}

void AVR32ToolChain::addClangTargetOptions(
    const ArgList &DriverArgs, ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadKind) const {
  DriverArgs.ClaimAllArgs(options::OPT_mpart_EQ);
  DriverArgs.ClaimAllArgs(options::OPT_mmcu_EQ);
  DriverArgs.ClaimAllArgs(options::OPT_masm_addr_pseudos);
  DriverArgs.ClaimAllArgs(options::OPT_rodata_writable);
}

Tool *AVR32ToolChain::buildLinker() const {
  return new tools::AVR32::Linker(*this);
}

void AVR32::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                 const InputInfo &Output,
                                 const InputInfoList &Inputs,
                                 const ArgList &Args,
                                 const char *LinkingOutput) const {
  const auto &TC = static_cast<const AVR32ToolChain &>(getToolChain());
  std::string Linker = TC.GetLinkerPath();
  ArgStringList CmdArgs;

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  std::string SysRoot = TC.computeSysRoot();
  if (!SysRoot.empty())
    CmdArgs.push_back(Args.MakeArgString("--sysroot=" + SysRoot));

  Args.AddAllArgs(CmdArgs, options::OPT_r);
  Args.AddAllArgs(CmdArgs, options::OPT_s);
  Args.AddAllArgs(CmdArgs, options::OPT_t);
  Args.AddAllArgs(CmdArgs, options::OPT_u);
  Args.AddAllArgs(CmdArgs, options::OPT_e);

  std::string Emulation = getAVR32Emulation(Args);
  if (!Emulation.empty()) {
    CmdArgs.push_back("-m");
    CmdArgs.push_back(Args.MakeArgString(Emulation));
  }

  if (!Args.hasArg(options::OPT_r))
    CmdArgs.push_back("--gc-sections");
  if (Args.hasFlag(options::OPT_mrelax, options::OPT_mno_relax, false))
    CmdArgs.push_back("--relax");

  Args.AddAllArgs(CmdArgs, options::OPT_L);
  TC.AddFilePathLibArgs(Args, CmdArgs);

  if (!Args.hasArg(options::OPT_T, options::OPT_r)) {
    if (std::optional<std::string> Script =
            getAVR32LinkerScript(SysRoot, Emulation))
      CmdArgs.push_back(Args.MakeArgString("-T" + *Script));
  }
  Args.AddAllArgs(CmdArgs, options::OPT_T);

  bool UseStartFiles = !Args.hasArg(options::OPT_nostdlib,
                                    options::OPT_nostartfiles, options::OPT_r);
  if (UseStartFiles) {
    CmdArgs.push_back(Args.MakeArgString(TC.GetFilePath("crt0.o")));
    CmdArgs.push_back(Args.MakeArgString(TC.GetFilePath("crti.o")));
    CmdArgs.push_back(Args.MakeArgString(TC.GetFilePath("crtbegin.o")));
  }

  AddLinkerInputs(TC, Inputs, Args, CmdArgs, JA);

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs,
                   options::OPT_r)) {
    CmdArgs.push_back("-lgcc");
    if (!Args.hasArg(options::OPT_nolibc))
      CmdArgs.push_back("-lc");
    CmdArgs.push_back("-lgcc");
  }

  if (UseStartFiles) {
    CmdArgs.push_back(Args.MakeArgString(TC.GetFilePath("crtend.o")));
    CmdArgs.push_back(Args.MakeArgString(TC.GetFilePath("crtn.o")));
  }

  Args.ClaimAllArgs(options::OPT_rodata_writable);
  Args.ClaimAllArgs(options::OPT_mpart_EQ);
  Args.ClaimAllArgs(options::OPT_mmcu_EQ);

  C.addCommand(std::make_unique<Command>(
      JA, *this, ResponseFileSupport::AtFileCurCP(), Args.MakeArgString(Linker),
      CmdArgs, Inputs, Output));
}
