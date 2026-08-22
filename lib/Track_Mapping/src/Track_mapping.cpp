#include "Track_mapping.hpp"

void Track::addTrackSection(int leftEncoderCount, int rightEncoderCount, int trackSection) {
  sections.emplace_back(leftEncoderCount, rightEncoderCount, trackSection);
}

void Track::printTrackInformation(Console& console) {
  console.println("Left Encoder Count, Right Encoder Count");
  for(const TrackSectionInformation& section : sections) {
    console.print(section.leftEncoderCount);
    console.print(",");
    console.print(section.rightEncoderCount);
    console.print(",");
  }
}
