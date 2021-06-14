//
// Created by johannes on 05.06.21.
//

#include <gtest/gtest.h>

#include "../src/index/GeometricTypes.h"

TEST(BoundingBox, First) {
  ad_geo::parse5Polygon("POLYGON(20 10 20 10 5)");

  ASSERT_TRUE(ad_geo::parseAxisRectancle("POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))"));
  ASSERT_TRUE(ad_geo::parseAxisRectancle("POLYGON((7.7305284 48.0516118,7.7305284 48.0516126,7.7305785 48.0516126,7.7305785 48.0516118,7.7305284 48.0516118))"));

}