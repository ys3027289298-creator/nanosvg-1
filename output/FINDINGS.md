# stop-opacity 验收结论

## 代码现状（src/ 未改动）

当前 HEAD 的 nanosvg 已完整实现 stop-opacity，全链路如下：

- 解析属性：src/nanosvg.h:1919-1920 经 nsvg__parseOpacity 读 stop-opacity，
  该函数在 src/nanosvg.h:1505-1511 将取值截断到 [0,1]。
- 解析 style：src/nanosvg.h:1865-1866 与 nsvg__parseStyle 复用同一 parseAttr，
  style="stop-opacity:..." 与 CSS class 样式表同样生效。
- 打包 alpha：src/nanosvg.h:2831-2832 把 opacity*255 写入 stop 颜色的 alpha 通道，
  缺省值 stopOpacity=1（src/nanosvg.h:2803）。
- 线性/径向插值：src/nanosvgrast.h:971-979 的 nsvg__lerpRGBA 对 r/g/b/a 同步插值；
  填充时 src/nanosvgrast.h:1044 与 1085 行将插值出的 ca 乘到覆盖 alpha 上。

因此无需修改源码即可满足验收第 7 条；本任务只新增 tests/stopopa/ 验证数据与脚本。

## 验证结果（output/report.txt）

- linear.svg：alpha 255 -> 0 单调平滑，16 点无跳变；与连续理论值偏差 <=8/255，
  来源是 256 级 LUT 的 int 截断与定点 lerp 量化，非透明度逻辑错误。
- radial.svg：8 个有效采样点与 LUT 理论值偏差全部为 0。
- single.svg：单停靠点不透明，16 点恒为 255，偏差 0。
- regression.svg：style 写法三停靠点（1 / 0.5 / 0），alpha 单调过渡，偏差 <=7/255。
- 无 stop-opacity 的渐变仅产生 alpha=255；仓库 example2 渲染 23.svg 与基线逐字节一致。

## 复现命令

    gcc -Isrc tests/stopopa/render.c -lm -o output/render
    python3 tests/stopopa/check.py | tee output/report.txt
