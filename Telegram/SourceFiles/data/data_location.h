/*
This file is part of rabbitGram Desktop,
the unofficial app based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/rabbitgramdesktop/rabbitgramdesktop/blob/dev/LEGAL
*/
#pragma once

namespace Data {

struct FileOrigin;

class LocationPoint {
public:
	LocationPoint() = default;
	explicit LocationPoint(const MTPDgeoPoint &point);

	[[nodiscard]] QString latAsString() const;
	[[nodiscard]] QString lonAsString() const;
	[[nodiscard]] MTPGeoPoint toMTP() const;

	[[nodiscard]] float64 lat() const;
	[[nodiscard]] float64 lon() const;
	[[nodiscard]] uint64 accessHash() const;

	[[nodiscard]] size_t hash() const;

	friend inline bool operator==(
			const LocationPoint &a,
			const LocationPoint &b) {
		return (a._lat == b._lat) && (a._lon == b._lon);
	}

	friend inline bool operator<(
			const LocationPoint &a,
			const LocationPoint &b) {
		return (a._lat < b._lat) || ((a._lat == b._lat) && (a._lon < b._lon));
	}

private:
	float64 _lat = 0;
	float64 _lon = 0;
	uint64 _access = 0;

};

[[nodiscard]] GeoPointLocation ComputeLocation(const LocationPoint &point);

} // namespace Data

namespace std {

template <>
struct hash<Data::LocationPoint> {
	size_t operator()(const Data::LocationPoint &value) const {
		return value.hash();
	}
};

} // namespace std
