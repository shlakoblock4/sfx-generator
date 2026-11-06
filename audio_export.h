#pragma once
#include <QString>
#include <vector>

bool writeWav(const QString& path, const std::vector<float>& data);
bool writeMp3(const QString& path, const std::vector<float>& data);
bool writeOgg(const QString& path, const std::vector<float>& data);