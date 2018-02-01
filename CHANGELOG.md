# Change Log

## [1.1.0](https://github.com/DigitalGlobe/GGD-gbdxm/tree/HEAD)

**Implemented enhancements:**

- Add model reference [\#44](https://github.com/DigitalGlobe/GGD-gbdxm/issues/44)
- If `--json` is supplied, `--type` on command line is ignored. [\#40](https://github.com/DigitalGlobe/GGD-gbdxm/issues/40)
- Remove unpack functionality from gbdxm public interface [\#39](https://github.com/DigitalGlobe/GGD-gbdxm/issues/39)
- Add Support for Packing/Unpacking Segmentation Models \(Caffe\) [\#21](https://github.com/DigitalGlobe/GGD-gbdxm/issues/21)
- File error message clarity [\#16](https://github.com/DigitalGlobe/GGD-gbdxm/issues/16)
- Allow install via APT [\#8](https://github.com/DigitalGlobe/GGD-gbdxm/issues/8)
- Change versioning to match new standard [\#6](https://github.com/DigitalGlobe/GGD-gbdxm/issues/6)
- Make --bbox default to global [\#5](https://github.com/DigitalGlobe/GGD-gbdxm/issues/5)
- Add model reference [\#45](https://github.com/DigitalGlobe/GGD-gbdxm/pull/45) ([rddesmond](https://github.com/rddesmond))
- Update model metadata version \(\#482\) [\#15](https://github.com/DigitalGlobe/GGD-gbdxm/pull/15) ([rddesmond](https://github.com/rddesmond))

**Fixed bugs:**

- Invalid test in assurance testing using custom data type [\#42](https://github.com/DigitalGlobe/GGD-gbdxm/issues/42)
- gbdxm pack should fail if --color-mode is not specified and model doesn't have one [\#35](https://github.com/DigitalGlobe/GGD-gbdxm/issues/35)
- Unable to set caffe output-layer through gbdxm pack using json for input [\#34](https://github.com/DigitalGlobe/GGD-gbdxm/issues/34)
- Optional argument --bounding-box is not optional when using -j to import arguments via json [\#31](https://github.com/DigitalGlobe/GGD-gbdxm/issues/31)
- DeepCore dependencies aren't translating to downstream dependencies [\#27](https://github.com/DigitalGlobe/GGD-gbdxm/issues/27)
- Double printed default. [\#23](https://github.com/DigitalGlobe/GGD-gbdxm/issues/23)
- Fix regression with labels in the json [\#18](https://github.com/DigitalGlobe/GGD-gbdxm/issues/18)
- Misleading Error Messages  [\#1](https://github.com/DigitalGlobe/GGD-gbdxm/issues/1)

**Closed issues:**

- Error message when using --gbdxm-file contains boost error [\#32](https://github.com/DigitalGlobe/GGD-gbdxm/issues/32)
- Update licensing to Apache 2.0 [\#10](https://github.com/DigitalGlobe/GGD-gbdxm/issues/10)

**Merged pull requests:**

- Gbdxm fixes for 1.1 [\#41](https://github.com/DigitalGlobe/GGD-gbdxm/pull/41) ([rddesmond](https://github.com/rddesmond))
- Added display and validation for model categories. [\#38](https://github.com/DigitalGlobe/GGD-gbdxm/pull/38) ([avitebskiy](https://github.com/avitebskiy))
- Fixed double-printed default for --output-dir [\#37](https://github.com/DigitalGlobe/GGD-gbdxm/pull/37) ([avitebskiy](https://github.com/avitebskiy))
- Add TensorFlow to supported types output string [\#30](https://github.com/DigitalGlobe/GGD-gbdxm/pull/30) ([LetThereBeDwight](https://github.com/LetThereBeDwight))
- Add required OpenCV find package for use with Caffe.  The change to n… [\#29](https://github.com/DigitalGlobe/GGD-gbdxm/pull/29) ([geovations](https://github.com/geovations))
- gbdxm issue \#27: Add Caffe to the list of dependencies for gbdxm to a… [\#28](https://github.com/DigitalGlobe/GGD-gbdxm/pull/28) ([geovations](https://github.com/geovations))
- Fixed error message being passed an std::string instead of a char\* [\#26](https://github.com/DigitalGlobe/GGD-gbdxm/pull/26) ([avitebskiy](https://github.com/avitebskiy))
- Explictly call std::move on returning a unique\_ptr [\#25](https://github.com/DigitalGlobe/GGD-gbdxm/pull/25) ([avitebskiy](https://github.com/avitebskiy))
- Fixed the "missing --version argument" bug. [\#24](https://github.com/DigitalGlobe/GGD-gbdxm/pull/24) ([avitebskiy](https://github.com/avitebskiy))
- Refactoring to catch up to all the latest DC changes. [\#22](https://github.com/DigitalGlobe/GGD-gbdxm/pull/22) ([avitebskiy](https://github.com/avitebskiy))
- Changes after Model and ModelMetadata refactoring. [\#20](https://github.com/DigitalGlobe/GGD-gbdxm/pull/20) ([avitebskiy](https://github.com/avitebskiy))
- Bugfix for regression to allow labels supplied in JSON [\#19](https://github.com/DigitalGlobe/GGD-gbdxm/pull/19) ([rddesmond](https://github.com/rddesmond))
- Fix for \#16, update the error messages around files [\#17](https://github.com/DigitalGlobe/GGD-gbdxm/pull/17) ([rddesmond](https://github.com/rddesmond))
- Add Versioning flags to 1.0.0 branch and add DeepCore version number … [\#14](https://github.com/DigitalGlobe/GGD-gbdxm/pull/14) ([geovations](https://github.com/geovations))
- Change version to 1.0.0 based on release branch. [\#13](https://github.com/DigitalGlobe/GGD-gbdxm/pull/13) ([geovations](https://github.com/geovations))
- gbdxm issue \#10. Update license to Apache 2.0 [\#11](https://github.com/DigitalGlobe/GGD-gbdxm/pull/11) ([geovations](https://github.com/geovations))
- Retry gbdxm issue \#8 on branch [\#9](https://github.com/DigitalGlobe/GGD-gbdxm/pull/9) ([geovations](https://github.com/geovations))
- Github issue \#6.  Change versioning from major.minor.build to major.m… [\#7](https://github.com/DigitalGlobe/GGD-gbdxm/pull/7) ([geovations](https://github.com/geovations))
- Use a default and simplify configure\_file [\#4](https://github.com/DigitalGlobe/GGD-gbdxm/pull/4) ([rddesmond](https://github.com/rddesmond))
- Adjustments after moving code to geometry. [\#3](https://github.com/DigitalGlobe/GGD-gbdxm/pull/3) ([avitebskiy](https://github.com/avitebskiy))
- Added find\_package JSONCPP [\#2](https://github.com/DigitalGlobe/GGD-gbdxm/pull/2) ([avitebskiy](https://github.com/avitebskiy))



\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*
