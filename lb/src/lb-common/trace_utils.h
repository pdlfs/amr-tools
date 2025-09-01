//
// Created by Ankush J on 4/17/23.
//

#pragma once

#include <pdlfs-common/env.h>

#include <regex>
#include <string>

#include "tools-common/logging.h"
#include "policy.h"

namespace amr {
class Utils {
 public:
  static int EnsureDir(pdlfs::Env* env, const std::string& dir_path) {
    pdlfs::Status s = env->CreateDir(dir_path.c_str()); if (s.ok()) { MLOG(MLOG_INFO, "\t- Created successfully.");
    } else if (s.IsAlreadyExists()) {
      MLOG(MLOG_INFO, "\t- Already exists.");
    } else {
      MLOG(MLOG_ERRO,
           "Failed to create output directory: %s (Reason: %s)",
           dir_path.c_str(), s.ToString().c_str());
      return -1;
    }

    return 0;
  }

  static std::vector<std::string> FilterByRegex(
      std::vector<std::string>& strings, std::string regex_pattern,
      std::vector<int> const& events) {
    std::vector<std::string> matches;
    const std::regex regex_obj(regex_pattern);

    for (auto& s : strings) {
      std::smatch match_obj;
      if (std::regex_match(s, match_obj, regex_obj)) {
        auto sub_match = match_obj[1];
        int sub_match_int = std::stoi(sub_match.str());
        if (std::find(events.begin(), events.end(), sub_match_int) ==
            events.end())
          continue;
        matches.push_back(s);
      }
    }
    return matches;
  }

  static std::vector<std::string> LocateTraceFiles(
      pdlfs::Env* env, const std::string& search_dir,
      const std::vector<int>& events) {
    MLOG(MLOG_INFO,
         "[SimulateTrace] Looking for trace files in: \n\t%s",
         search_dir.c_str());

    std::vector<std::string> all_files;
    env->GetChildren(search_dir.c_str(), &all_files);

    MLOG(MLOG_DBG2, "Enumerating directory: %s",
         search_dir.c_str());
    for (auto& f : all_files) {
      MLOG(MLOG_DBG2, "- File: %s", f.c_str());
    }

    // Disabled \d as we only need two specific evts
    // std::vector<std::string> regex_patterns = {
    // R"(prof\.merged\.evt(\d+)\.csv)",
    // R"(prof\.merged\.evt(\d+)\.mini\.csv)",
    // R"(prof\.aggr\.evt(\d+)\.csv)",
    // };

    std::vector<std::string> regex_patterns = {
        R"(prof\.merged\.evt(\d+)\.csv)",
        R"(prof\.merged\.evt(\d+)\.mini\.csv)",
        R"(prof\.aggr\.evt(\d+)\.csv)",
        R"(evt(\d+)\.mat\.bin)",
    };

    std::vector<std::string> relevant_files;
    for (auto& pattern : regex_patterns) {
      MLOG(MLOG_DBG2, "Searching by pattern: %s (nevents: %zu)",
           pattern.c_str(), events.size());
      relevant_files = FilterByRegex(all_files, pattern, events);

      for (auto& f : relevant_files) {
        MLOG(MLOG_DBG2, "- Match: %s", f.c_str());
      }

      if (!relevant_files.empty()) break;
    }

    if (relevant_files.empty()) {
      ABORT("no trace files found!");
    }

    std::vector<std::string> all_fpaths;

    for (auto& f : relevant_files) {
      std::string full_path = std::string(search_dir) + "/" + f;
      MLOG(MLOG_INFO, "[ProfSetReader] Adding trace file: %s",
           full_path.c_str());
      all_fpaths.push_back(full_path);
    }

    return all_fpaths;
  }

  static ProfTimeCombinePolicy ParseProfTimeCombinePolicy(
      const char* policy_str) {
    std::string policy(policy_str);

    if (policy == "first") {
      return ProfTimeCombinePolicy::kUseFirst;
    } else if (policy == "last") {
      return ProfTimeCombinePolicy::kUseLast;
    } else if (policy == "add") {
      return ProfTimeCombinePolicy::kAdd;
    } else {
      MLOG(MLOG_ERRO, "Invalid time combine policy: %s",
           policy.c_str());
      ABORT("Invalid time combine policy");
    }

    return ProfTimeCombinePolicy::kAdd;
  }

  static std::string GetProfTimeCombinePolicyStr(ProfTimeCombinePolicy policy) {
    switch (policy) {
      case ProfTimeCombinePolicy::kUseFirst:
        return "first";
      case ProfTimeCombinePolicy::kUseLast:
        return "last";
      case ProfTimeCombinePolicy::kAdd:
        return "add";
      default:
        ABORT("Invalid time combine policy");
    }

    return "<unknown>";
  }

  static void WriteToFile(pdlfs::Env* env, const std::string& fpath,
                          const std::string& content) {
    pdlfs::Status s = pdlfs::Status::OK();

    // ensure parent dir exists, create if it doesn't
    auto parent_dir = fpath.substr(0, fpath.find_last_of("/"));
    if (!env->FileExists(parent_dir.c_str())) {
      s = env->CreateDir(parent_dir.c_str());
      if (!s.ok()) {
        MLOG(MLOG_ERRO, "Error creating dir: %s",
             parent_dir.c_str());
        return;
      }
    }

    pdlfs::WritableFile* fh;

    s = env->NewWritableFile(fpath.c_str(), &fh);
    if (!s.ok()) {
      MLOG(MLOG_ERRO, "Error opening file: %s", fpath.c_str());
      return;
    }

    s = fh->Append(content);
    if (!s.ok()) {
      MLOG(MLOG_ERRO, "Error opening file: %s", fpath.c_str());
    }

    s = fh->Close();
    if (!s.ok()) {
      MLOG(MLOG_ERRO, "Error opening file: %s", fpath.c_str());
    }
  }
};
}  // namespace amr
