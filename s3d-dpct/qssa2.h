#ifndef QSSA2_H
#define QSSA2_H

#include <CL/sycl.hpp>
#include <dpct/dpct.hpp>
#include "S3D.h"

template <class real>
void
LAUNCH_BOUNDS (QSSA2_THRD, QSSA2_BLK)
qssa2_kernel(real* RESTRICT RF, real* RESTRICT RB, const real* RESTRICT A,
             sycl::nd_item<3> item_ct1)
{

    register real xq4 = A(4,0);
    register real xq3 = A(3,0) +A(3,4)*xq4;
    register real xq7 = A(7,0) +A(7,4)*xq4 +A(7,3)*xq3;
    register real xq2 = A(2,0) +A(2,4)*xq4 +A(2,3)*xq3 +A(2,7)*xq7;
    register real xq1 = A(1,0) +A(1,4)*xq4 +A(1,3)*xq3 +A(1,7)*xq7 +A(1,2)*xq2;
    register real xq8 = A(8,0) +A(8,4)*xq4 +A(8,3)*xq3;
    register real xq6 = A(6,0) +A(6,3)*xq3 +A(6,7)*xq7 +A(6,2)*xq2;
    register real xq9 = A(9,0) +A(9,4)*xq4 +A(9,7)*xq7;
    register real xq5 = A(5,0) +A(5,3)*xq3;
    register real xq10 = A(10,0) +A(10,8)*xq8;

    RF(34) = RF(34)*xq1;
    RF(35) = RF(35)*xq1;
    RB(35) = RB(35)*xq4;
    RF(36) = RF(36)*xq1;
    RB(36) = RB(36)*xq2;
    RF(37) = RF(37)*xq1;
    RF(38) = RF(38)*xq1;
    RB(38) = RB(38)*xq4;
    RF(39) = RF(39)*xq1;
    RF(40) = RF(40)*xq1;
    RB(40) = RB(40)*xq4;
    RF(41) = RF(41)*xq4;
    RF(42) = RF(42)*xq4;
    RF(43) = RF(43)*xq4;
    RF(44) = RF(44)*xq4;
    RF(45) = RF(45)*xq4;
    RF(46) = RF(46)*xq4;
    RF(47) = RF(47)*xq4;
    RF(48) = RF(48)*xq2;
    RF(49) = RF(49)*xq2;
    RF(50) = RF(50)*xq2;
    RB(50) = RB(50)*xq4;
    RF(51) = RF(51)*xq2;
    RB(51) = RB(51)*xq4;
    RF(52) = RF(52)*xq2;
    RF(53) = RF(53)*xq2;
    RF(54) = RF(54)*xq2;
    RB(54) = RB(54)*xq1;
    RF(55) = RF(55)*xq2;
    RF(56) = RF(56)*xq2;
    RF(59) = RF(59)*xq3;
    RB(59) = RB(59)*xq2;
    RF(60) = RF(60)*xq3;
    RB(60) = RB(60)*xq1;
    RF(61) = RF(61)*xq3;
    RF(62) = RF(62)*xq3;
    RB(62) = RB(62)*xq4;
    RF(63) = RF(63)*xq3;
    RF(64) = RF(64)*xq3;
    RF(65) = RF(65)*xq3;
    RF(66) = RF(66)*xq3;
    RF(67) = RF(67)*xq3;
    RB(67) = RB(67)*xq2;
    RF(68) = RF(68)*xq3;
    RB(68) = RB(68)*xq2;
    RF(69) = RF(69)*xq3;
    RB(69) = RB(69)*xq2;
    RF(70) = RF(70)*xq3;
    RB(71) = RB(71)*xq5;
    RB(72) = RB(72)*xq4;
    RB(73) = RB(73)*xq4;
    RB(74) = RB(74)*xq4;
    RB(75) = RB(75)*xq4;
    RB(76) = RB(76)*xq4;
    RF(77) = RF(77)*xq1;
    RB(80) = RB(80)*xq2;
    RB(81) = RB(81)*xq3;
    RB(82) = RB(82)*xq5;
    RB(85) = RB(85)*xq5;
    RF(87) = RF(87)*xq1;
    RB(87) = RB(87)*xq7;
    RF(88) = RF(88)*xq4;
    RF(89) = RF(89)*xq4;
    RB(90) = RB(90)*xq4;
    RF(91) = RF(91)*xq2;
    RF(92) = RF(92)*xq3;
    RB(94) = RB(94)*xq8;
    RF(96) = RF(96)*xq5;
    RF(97) = RF(97)*xq5;
    RF(98) = RF(98)*xq5;
    RB(98) = RB(98)*xq3;
    RF(99) = RF(99)*xq5;
    RF(100) = RF(100)*xq5;
    RF(101) = RF(101)*xq5;
    RF(105) = RF(105)*xq1;
    RF(106) = RF(106)*xq2;
    RF(107) = RF(107)*xq3;
    RB(108) = RB(108)*xq3;
    RF(111) = RF(111)*xq1;
    RF(112) = RF(112)*xq2;
    RB(112) = RB(112)*xq7;
    RB(114) = RB(114)*xq6;
    RF(115) = RF(115)*xq7;
    RB(117) = RB(117)*xq2;
    RF(120) = RF(120)*xq4;
    RB(120) = RB(120)*xq7;
    RF(122) = RF(122)*xq6;
    RF(123) = RF(123)*xq6;
    RB(123) = RB(123)*xq2;
    RF(124) = RF(124)*xq6;
    RF(125) = RF(125)*xq6;
    RB(125) = RB(125)*xq2;
    RB(126) = RB(126)*xq9;
    RB(130) = RB(130)*xq2;
    RF(132) = RF(132)*xq7;
    RF(133) = RF(133)*xq7;
    RF(134) = RF(134)*xq7;
    RB(134) = RB(134)*xq6;
    RF(135) = RF(135)*xq7;
    RF(136) = RF(136)*xq7;
    RF(137) = RF(137)*xq7;
    RF(138) = RF(138)*xq7;
    RF(139) = RF(139)*xq7;
    RB(139) = RB(139)*xq9;
    RF(140) = RF(140)*xq7;
    RB(140) = RB(140)*xq4;
    RF(141) = RF(141)*xq7;
    RB(141) = RB(141)*xq9;
    RF(142) = RF(142)*xq7;
    RF(144) = RF(144)*xq7;
    RF(145) = RF(145)*xq7;
    RF(146) = RF(146)*xq7;
    RF(147) = RF(147)*xq9;
    RF(148) = RF(148)*xq9;
    RF(149) = RF(149)*xq9;
    RB(149) = RB(149)*xq4;
    RF(150) = RF(150)*xq9;
    RF(151) = RF(151)*xq9;
    RF(152) = RF(152)*xq9;
    RF(153) = RF(153)*xq9;
    RF(154) = RF(154)*xq9;
    RB(155) = RB(155)*xq6;
    RB(156) = RB(156)*xq8;
    RB(157) = RB(157)*xq7;
    RB(158) = RB(158)*xq7;
    RB(159) = RB(159)*xq4;
    RB(160) = RB(160)*xq2;
    RB(161) = RB(161)*xq7;
    RB(162) = RB(162)*xq7;
    RF(164) = RF(164)*xq4;
    RB(164) = RB(164)*xq8;
    RF(165) = RF(165)*xq2;
    RF(166) = RF(166)*xq3;
    RB(166) = RB(166)*xq6;
    RF(167) = RF(167)*xq3;
    RB(168) = RB(168)*xq7;
    RB(169) = RB(169)*xq10;
    RF(170) = RF(170)*xq8;
    RF(171) = RF(171)*xq8;
    RF(172) = RF(172)*xq8;
    RF(173) = RF(173)*xq8;
    RF(174) = RF(174)*xq8;
    RF(175) = RF(175)*xq8;
    RF(176) = RF(176)*xq8;
    RF(177) = RF(177)*xq8;
    RF(178) = RF(178)*xq8;
    RB(180) = RB(180)*xq8;
    RB(181) = RB(181)*xq8;
    RB(182) = RB(182)*xq8;
    RF(183) = RF(183)*xq3;
    RB(183) = RB(183)*xq8;
    RB(184) = RB(184)*xq8;
    RB(186) = RB(186)*xq6;
    RB(188) = RB(188)*xq7;
    RF(189) = RF(189)*xq4;
    RB(190) = RB(190)*xq10;
    RF(199) = RF(199)*xq10;
    RB(199) = RB(199)*xq8;
    RF(200) = RF(200)*xq10;
    RF(201) = RF(201)*xq10;
    RB(201) = RB(201)*xq8;
    RF(202) = RF(202)*xq10;
    RF(203) = RF(203)*xq10;
    RF(204) = RF(204)*xq10;
    RB(204) = RB(204)*xq8;
    RF(205) = RF(205)*xq10;
}

#endif // QSSA2_H


