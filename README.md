# Numerical-Analysis_Adaptive-Interpolation
이미지 내 지역적인 특성을 이용하여 적응형 필터를 생성, 이를 사용한 보간 (reconstruction)

  + 256x256 크기의 이미지를 가로세로 2배씩 upsampling
  + 화소마다 가로, 세로, 대각선 방향으로의 edge 방향성, 세기를 탐색하여 분류
  + 최소제곱법으로 각 그룹별 필터를 최적화
    - Ground truth 이미지와 입력 이미지의 픽셀 간 평균 제곱 오차를 최소화
