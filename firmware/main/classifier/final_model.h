#pragma once
#include <cstdarg>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class DecisionTree {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        if (x[5] <= 1.2740535736083984) {
                            if (x[1] <= -0.13028906285762787) {
                                if (x[0] <= -0.4393535256385803) {
                                    if (x[2] <= 0.30287694931030273) {
                                        return 1;
                                    }

                                    else {
                                        return 0;
                                    }
                                }

                                else {
                                    if (x[6] <= 0.6548986732959747) {
                                        return 0;
                                    }

                                    else {
                                        return 0;
                                    }
                                }
                            }

                            else {
                                if (x[3] <= 0.034728413447737694) {
                                    if (x[0] <= -0.09574609249830246) {
                                        return 0;
                                    }

                                    else {
                                        return 0;
                                    }
                                }

                                else {
                                    if (x[4] <= 4.834182500839233) {
                                        return 0;
                                    }

                                    else {
                                        return 1;
                                    }
                                }
                            }
                        }

                        else {
                            if (x[5] <= 6.611015319824219) {
                                if (x[3] <= 0.49433112144470215) {
                                    if (x[1] <= -0.9281074404716492) {
                                        return 0;
                                    }

                                    else {
                                        return 1;
                                    }
                                }

                                else {
                                    if (x[0] <= -0.4910527467727661) {
                                        return 1;
                                    }

                                    else {
                                        return 0;
                                    }
                                }
                            }

                            else {
                                return 1;
                            }
                        }
                    }

                    /**
                    * Predict readable class name
                    */
                    const char* predictLabel(float *x) {
                        return idxToLabel(predict(x));
                    }

                    /**
                    * Convert class idx to readable name
                    */
                    const char* idxToLabel(uint8_t classIdx) {
                        switch (classIdx) {
                            case 0:
                            return "Noise";
                            case 1:
                            return "Throwing";
                            default:
                            return "Houston we have a problem";
                        }
                    }

                protected:
                };
            }
        }
    }