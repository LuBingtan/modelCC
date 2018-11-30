#include <experimental/filesystem>
#include "VideoWriter.h"
#include "utils.h"
#include <mutex> 

VideoWriter::VideoWriter(bool orig, bool debug, std::string& path, std::string& channel): writeOrig(orig), writeDebug(debug), baseDir(path), ch(channel) {
  currentHourDebug = time_t_to_string_hour(time(0));
  currentHourOrig = currentHourDebug;
}

VideoWriter::~VideoWriter() {
    close();
}

 std::mutex g_i_mutex;

void deleteOldVideos(std::string& path, int keep) {
        std::lock_guard<std::mutex> lock(g_i_mutex);
        for (auto & p : std::experimental::filesystem::directory_iterator(path)) {
        if (std::experimental::filesystem::is_regular_file(p) && p.path().extension() == ".avi") {
            auto ftime = std::experimental::filesystem::last_write_time(p);
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::hours(keep * 24);
            if (now - ftime > duration) {
                std::experimental::filesystem::remove(p);
                std::cout << "delete: " << p << std::endl;
            }
        }
    }
}

void VideoWriter::writeVideo(cv::Mat& src, 
                            cv::VideoWriter** writerp, 
                            const std::string surfix, 
                            std::string* currentHour,
                            bool enable, 
                            int fps) {
  
  std::string hour = time_t_to_string_hour(time(0));

  std::string outPath = baseDir + "/" + ch + "-" + hour + surfix;
  
  if (enable && (*writerp) == nullptr) {
     (*writerp) = new cv::VideoWriter(outPath, 
                                CV_FOURCC('D','I','V','X'),
                                fps, 
                                cv::Size(src.cols, src.rows), 
                                true);
    deleteOldVideos(baseDir, keepDays);
    std::cout << "new file: " << outPath << std::endl;
    if (!(*writerp) -> isOpened()) {
        std::cout << "fail to open " << outPath << std::endl;
        return;
    }
  }
    
  if (*writerp) {
      if (hour != *currentHour) {
        *currentHour = hour;
        (*writerp) -> release();
        std::string newPath = baseDir + "/" + ch + "-" + hour + surfix;
        (*writerp) = new cv::VideoWriter(newPath, 
                                CV_FOURCC('D','I','V','X'),
                                fps, 
                                cv::Size(src.cols, src.rows), 
                                true);
        deleteOldVideos(baseDir, keepDays);
        std::cout << "new file: " << newPath << std::endl;
        if (!(*writerp) -> isOpened()) {
          std::cout << "fail to open " << newPath << std::endl;
          return;
        }
      }

      (*writerp) -> write(src);
  }
}

const std::string ORIG_AVI = ".orig.avi";
const std::string DEBUG_AVI = ".debug.avi";


void VideoWriter::writeOrigVideo(cv::Mat& src) {
    writeVideo (src, &origWriter, ORIG_AVI , &currentHourOrig, writeOrig, 25);
}

void VideoWriter::writeDebugVideo(cv::Mat& src) {
    writeVideo (src, &debugWriter, DEBUG_AVI, &currentHourDebug, writeDebug, 5);
}

int VideoWriter::close() {
    if (origWriter) {
        origWriter -> release();
    }

    if (debugWriter) {
        debugWriter -> release();
    }

    return 0;
}
