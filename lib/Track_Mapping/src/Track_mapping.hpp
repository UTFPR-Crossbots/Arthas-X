#ifndef _Track_mapping_h_
#define _Track_mapping_h_

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <vector>

class TrackSectionInformation {
  public:
    int leftEncoderCount;
    int rightEncoderCount;
    int trackSection;

   TrackSectionInformation(double lwd, double rwd, int si)
    : leftEncoderCount(lwd), rightEncoderCount(rwd), trackSection(si) {}
};

class Track {

  public:
    void addTrackSection(int leftEncoderCount, int rightEncoderCount, int trackSection);
    void printTrackInformation(BluetoothSerial& SerialBT);

  private:
    std::vector<TrackSectionInformation> sections;

};

#endif