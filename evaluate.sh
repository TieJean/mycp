# declare -a TESTNAMES=("SingleLargeFileTest BTreeSmallFilesTest BTreeMediumFilesTest BTreeLargeFilesTest DTreeMediumFilesTest")   
declare -a TESTNAMES=("Single512MFileTest Single1GFileTest Single2GFileTest")   

# DATA_DIR="$(pwd)/data/"
DATA_DIR="/home/taijing/project/data/"

src_path=""
dst_path=""
for test_name in ${TESTNAMES[@]}; do
    src_path="${DATA_DIR}${test_name}/src/"
    dst_path="${DATA_DIR}${test_name}/dst/"
    echo "---${test_name}---"

    echo "cp: cp -r ${src_path} ${dst_path}"
    ./bin/flush_main
    rm -rf ${dst_path}*
    time cp -r ${src_path} ${dst_path}

done

for test_name in ${TESTNAMES[@]}; do
    src_path="${DATA_DIR}${test_name}/src/"
    dst_path="${DATA_DIR}${test_name}/dst/"
    echo "---${test_name}---"

    echo "mycp: ./bin/main --src=${src_path} --dst=${dst_path}"
    ./bin/flush_main
    rm -rf ${dst_path}*
    time ./bin/main --src=${src_path} --dst=${dst_path}
done