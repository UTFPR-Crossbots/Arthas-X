#include "Track_mapping.hpp"

void Track::addTrackSection(int leftEncoderCount, int rightEncoderCount, int trackSection) {
  sections.emplace_back(leftEncoderCount, rightEncoderCount, trackSection);
}

void Track::printTrackInformation(BluetoothSerial& SerialBT) {
  SerialBT.println("Left Encoder Count, Right Encoder Count");
  for(const TrackSectionInformation& section : sections) {
    SerialBT.print(section.leftEncoderCount);
    SerialBT.print(",");
    SerialBT.print(section.rightEncoderCount);
    SerialBT.print(",");
  }
}