clang++ -std=c++11 -stdlib=libc++ -O3 list_traversal.cpp -o list_traversal
./list_traversal | tee list_traversal.data

gnuplot plot.plg
