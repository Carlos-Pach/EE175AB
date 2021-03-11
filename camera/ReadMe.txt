for reference:
https://github.com/EdjeElectronics/TensorFlow-Object-Detection-API-Tutorial-Train-Multiple-Objects-Windows-10
=================================================================================
PATH TO OBJECT DETECTION FOLDER: C:\tensorflow1\models\research\object_detection
=================================================================================
=================================================================================
VERSIONS:
TF1: 1.13.1 (updated to 1.14.1 to use model_main) (library for high scale computer and ML)(check by pip list | grep tensorflow) or with pip3
Python: 3.7.3 (64bit in Windows, 32bit in Pi)
protobuf: 3.13  (check by: protoc --version)
(here is 3.14 https://github.com/protocolbuffers/protobuf/releases/tag/v3.14.0 )
opencv 4.5.1.48
NUMPY: conda install numpy=1.17.4
picamera=1.13 (v2: https://picamera.readthedocs.io/en/release-1.13/faq.html)
------------------------------------------------------------
USE:
============================================================================================
--------------------------------------------------------------------------------------------
V I R T U A L   E N V: easily keep different versions of programs, etc.:
======================
in tensorflow1 folder we stored our virtual environement called tensorflow 1 (also)
----create by: -------------------------------------
install with: python -m pip name_of_thing_i_want_to_install
conda create --name thenameofvenv python=3.7.3
conda activate nameofvenv
conda deactivate
(remove by:
conda env remove --name myenv.
To verify that the environment was removed, in your terminal window or an Anaconda Prompt, run:
conda info --envs
)
!!!!!!!!!!!!!!!!IMPORTANT!!!!!!!!!!!!!!!!!
NEED TO SET PYTOHNPATH everytime I re-enter virtual env, unless I set as default in .bashrc (easier in pi, linux)
set by:  
set PYTHONPATH=C:\tensorflow1\models;C:\tensorflow1\models\research;C:\tensorflow1\models\research\slim
set PYTHONPATH=C:\tensorflow14\models;C:\tensorflow14\models\research;C:\tensorflow14\models\research\slim
check by :   echo %PYTHONPATH%
(optionally:set PATH=%PATH%;PYTHONPATH )


for exporting tf:
(setup)
	conda activate tensorflow-build
	(( IN C:/))
	set PATH=%PATH%;C:\msys64\usr\bin
(exporting)
	set CONFIG_FILE=C:\\tensorflow1\models\research\object_detection\training\ssd_mobilenet_v2_quantized_300x300_coco.config
	set CHECKPOINT_PATH=C:\\tensorflow1\models\research\object_detection\images\model.ckpt-3295
	set OUTPUT_DIR=C:\\tensorflow1\models\research\object_detection\TFLite_model
	python export_tflite_ssd_graph.py --pipeline_config_path=%CONFIG_FILE% --trained_checkpoint_prefix=%CHECKPOINT_PATH% --output_directory=%OUTPUT_DIR% --add_postprocessing_op=true
		then in the FLite_model folder download tflite_graph.pb to Drive and go to:
		https://colab.research.google.com/drive/1DolVXkn1w0ADa4B_jAuCpjSnyRSApH7D?authuser=2#scrollTo=oBjfejA4FgTf
		to get the detect file!
--------------------------------------------------------------------------------------------

INSTALLATION:
When first installing Anaconda:
Anaconda: defaults to installing in  C:\Users\Kelly Duong 
but we shouldn't install anaconda in filename with spaces, so I installed in C:\

Anaconda administrator automatically opens to: C:\Windows\system32>>
(so cd .. back to tensorflow1 folder in C:\ to enter venv)


tensorflow1 is for cpu
for gpu I don't have a nvdia card so cannot use... reduces training time by ~12x
(reason why I can't use locally: https://stackoverflow.com/questions/58367880/can-i-able-to-detect-object-with-intelr-uhd-graphics-620 )
NEW:
UPDATED TO TF1.14.1 SO NEW VENV NAME: tensorflow14  , please set path appropriately too below
------------------------------------------------------------------
ISSUES:

	TRAINING SETUP:
		During this protoc, it won't be able to create a new file for calibration, so leave it out of the code. (or in?)
		protoc --python_out=. .\object_detection\protos\calibration.proto


	EVAL SETUP: (Pycoco tools for testing):
		ex. (from file eval.py)
			ModuleNotFoundError: No module named 'pycocotools._mask'
		or 
			ImportError: No module named pycocotools
follow cocoapi download, pip install git+https://github.com/philferriere/cocoapi.git#subdirectory=PythonAPI
then goto tf1/models/research, replace pycocotools folder with different name and run
(cont.) Now issue with detecting object 64/numpy.float64? (or something to do with int, etc.)
	IN: something like File "C:\condaconda64\envs\tensorflow1\Lib\site-packages\numpy\core"
	num = operator.index(num)
	Go to your numpy and change the line to:
	num = operator.index(int(num))
		(if not, other reccomend downgrading numpy version, rn have 1.19 so:   
		pip install numpy==1.17.5 OR IF NOT: conda install numpy=1.17.4)
		then try eval.py again! For some reason, many problems were fixed with anaconda's conda install


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
TRAIN / EVALUATION :

train:
	-use model_main.py for eval+train, or do train.py & eval.py seperately
	python object_detection/model_main.py \
    	--pipeline_config_path=/training/ssd_mobilenet_v2_quantized_300x300_coco.config\
    	--model_dir=\training \ 
    	--num_train_steps=50000\
    	--sample_1_of_n_eval_examples=1\
    	--alsologtostderr
	then (also maybe change batch size to smaller # if less memory)

	#in object detection folder:
	#Can use model_main script to run train and eval at the same time! rather than one at a time (train.py and eval.py from the legacy folder)
	#python model_main.py --pipeline_config_path=/path/to/pipeline_file --model_dir=/path/to/output_results --checkpoint_dir=/path/to/directory_holding_checkpoint --run_once=True
	#python models/research/object_detection/model_main.py --pipeline_config_path=/training/ssd_mobilenet_v2_quantized_300x300_coco.config --checkpoint_dir=/training/model.ckpt-75 --run_once=True

python model_main.py --pipeline_config_path=training\ssd_mobilenet_v2_quantized_300x300_coco.config --model_dir=images --num_train_steps=50000 --sample_1_of_n_eval_examples=1 --alsologtostderr

 (Checkpoint does not seem to work-> )--checkpoint_dir=\images\model.ckpt-1345 



eval:
TENSORBOARD:  (graph visualization during training time):
	open another terminal window: (MAKE sure to goto venv, then cd into obj detection)

	tensorboard --logdir=images --host localhost --port 8088
	(in browser) http://localhost:8088/

Error?
	go to C:\condaconda64\envs\tensorflow1\Lib\site-packages\tensorboard\manager.py
	(change these lines:)
	serialize=lambda dt: int(dt.strftime("%S")),
    	#serialize=lambda dt: int(
    	#    (dt - datetime.datetime.fromtimestamp(0)).total_seconds()),
)



()()()()()()( 8 total classes ) ()()()()()()
First train: 
train:452, test: 132 = total 584 with ~23% split
loss lowered from 30 to 2 over  few hours, I also put 264 in eval_config, num examples,ssd_config; when it should be 132? hmm

Second train:
284 more pics, so~63% increase , per class increase is around 35.5 pics more
train:736, test:187 = 923 pictureswith~ 20% split

()()()()()()()()()()()()()()()
==============================================================================================
==============================================================================================
DATASETS:  
http://ufldl.stanford.edu/housenumbers/

Visual Genome's Dataset of images: (for humans, background pictures, some plants, etc.)
http://visualgenome.org/api/v0/api_home.html
Fudan/Penn humans dataset:
https://github.com/mahavird/Person-Detector-Dataset

Dat Tran's raccoon dataset:
https://github.com/datitran/raccoon_dataset
https://towardsdatascience.com/how-to-train-your-own-object-detector-with-tensorflows-object-detector-api-bec72ecfe1d9

created squirrel, potted plant dataset

augmentation:
https://github.com/tensorflow/models/blob/master/research/object_detection/builders/preprocessor_builder_test.py
https://scholarworks.sjsu.edu/cgi/viewcontent.cgi?article=1613&context=etd_projects
^ SJSU implemented various data augmentations that worked well on my dataset too like random crop pad, random black patches, etc. 
==================================================================================
RASPBERRY PI:

"df -h" to check for mem used in raspi
SO FAR: on my 64 GB mem, I've used 9.5G so 47G left (17% used) 

list all virtualenvironemntsL
brief usage:

$ lsvirtualenv -b
long usage:

$ lsvirtualenv -l

(while testing for a few minutes, it went from ~47 to 63 degrees! But apparently I should only start to worry if it goes for 80+-100 for a while)

`````````````````````````````````````````````````````````````````````
PROBLEM WITH INSTALLING BAZEL, etc. TO CONVERT TO TFLITE SO:
followed:in issues:
https://github.com/EdjeElectronics/TensorFlow-Lite-Object-Detection-on-Android-and-Raspberry-Pi/issues/40
https://colab.research.google.com/drive/1DolVXkn1w0ADa4B_jAuCpjSnyRSApH7D?authuser=2#scrollTo=oBjfejA4FgTf
(if doesnt open in laptop, check disabling otherextensions in browser ex. if it appears ok in igcognito/phone)

error looked like ths:

(tensorflow-build) C:\tensorflow-build\tensorflow>bazel build --config=opt //tensorflow/tools/pip_package:build_pip_package
WARNING: C:/tensorflow-build/tensorflow/tensorflow/python/BUILD:3469:1: in py_library rule //tensorflow/python:standard_ops: target '//tensorflow/python:standard_ops' depends on deprecated target '//tensorflow/python/ops/distributions:distributions': TensorFlow Distributions has migrated to TensorFlow Probability (https://github.com/tensorflow/probability). Deprecated copies remaining in tf.distributions will not receive new features, and will be removed by early 2019. You should update all usage of `tf.distributions` to `tfp.distributions`.
WARNING: C:/tensorflow-build/tensorflow/tensorflow/python/BUILD:102:1: in py_library rule //tensorflow/python:no_contrib: target '//tensorflow/python:no_contrib' depends on deprecated target '//tensorflow/python/ops/distributions:distributions': TensorFlow Distributions has migrated to TensorFlow Probability (https://github.com/tensorflow/probability). Deprecated copies remaining in tf.distributions will not receive new features, and will be removed by early 2019. You should update all usage of `tf.distributions` to `tfp.distributions`.
WARNING: C:/tensorflow-build/tensorflow/tensorflow/contrib/metrics/BUILD:16:1: in py_library rule //tensorflow/contrib/metrics:metrics_py: target '//tensorflow/contrib/metrics:metrics_py' depends on deprecated target '//tensorflow/python/ops/distributions:distributions': TensorFlow Distributions has migrated to TensorFlow Probability (https://github.com/tensorflow/probability). Deprecated copies remaining in tf.distributions will not receive new features, and will be removed by early 2019. You should update all usage of `tf.distributions` to `tfp.distributions`.
WARNING: C:/tensorflow-build/tensorflow/tensorflow/contrib/learn/BUILD:17:1: in py_library rule //tensorflow/contrib/learn:learn: target '//tensorflow/contrib/learn:learn' depends on deprecated target '//tensorflow/contrib/session_bundle:exporter': No longer supported. Switch to SavedModel immediately.
WARNING: C:/tensorflow-build/tensorflow/tensorflow/contrib/learn/BUILD:17:1: in py_library rule //tensorflow/contrib/learn:learn: target '//tensorflow/contrib/learn:learn' depends on deprecated target '//tensorflow/contrib/session_bundle:gc': No longer supported. Switch to SavedModel immediately.
WARNING: C:/tensorflow-build/tensorflow/tensorflow/contrib/bayesflow/BUILD:17:1: in py_library rule //tensorflow/contrib/bayesflow:bayesflow_py: target '//tensorflow/contrib/bayesflow:bayesflow_py' depends on deprecated target '//tensorflow/contrib/distributions:distributions_py': TensorFlow Distributions has migrated to TensorFlow Probability (https://github.com/tensorflow/probability). Deprecated copies remaining in tf.contrib.distributions are unmaintained, unsupported, and will be removed by late 2018. You should update all usage of `tf.contrib.distributions` to `tfp.distributions`.
WARNING: C:/tensorflow-build/tensorflow/tensorflow/contrib/BUILD:12:1: in py_library rule //tensorflow/contrib:contrib_py: target '//tensorflow/contrib:contrib_py' depends on deprecated target '//tensorflow/contrib/distributions:distributions_py': TensorFlow Distributions has migrated to TensorFlow Probability (https://github.com/tensorflow/probability). Deprecated copies remaining in tf.contrib.distributions are unmaintained, unsupported, and will be removed by late 2018. You should update all usage of `tf.contrib.distributions` to `tfp.distributions`.
INFO: Analysed target //tensorflow/tools/pip_package:build_pip_package (1 packages loaded, 1 target configured).
INFO: Found 1 target...
INFO: From Compiling external/com_google_absl/absl/base/dynamic_annotations.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/strings/internal/utf8.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/strings/internal/ostringstream.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/base/internal/spinlock_wait.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From ProtoCompile tensorflow/core/protobuf/replay_log.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
tensorflow/core/protobuf/replay_log.proto: warning: Import tensorflow/core/protobuf/cluster.proto but not used.
tensorflow/core/protobuf/replay_log.proto: warning: Import tensorflow/core/framework/graph.proto but not used.
INFO: From ProtoCompile tensorflow/core/kernels/boosted_trees/boosted_trees.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/python/framework/cpp_shape_inference.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/protobuf/eager_service.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/protobuf/tpu/tpu_embedding_output_layout.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/protobuf/tpu/tpu_embedding_configuration.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/debug/debugger_event_metadata.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/profiler/profile.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/profiler/tfprof_options.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/profiler/tfprof_log.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/profiler/profiler_service.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/debug/debug_service.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/profiler/profiler_analysis.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/protobuf/worker.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/protobuf/master.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From ProtoCompile tensorflow/core/protobuf/tpu/optimization_parameters.pb.h:
bazel-out/x64_windows-opt/genfiles/external/protobuf_archive/src: warning: directory does not exist.
INFO: From Compiling external/com_google_absl/absl/debugging/internal/address_is_readable.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/debugging/internal/elf_mem_image.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/debugging/internal/vdso_support.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/debugging/internal/demangle.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/barrier.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/blocking_counter.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/internal/create_thread_identity.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/internal/waiter.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/internal/per_thread_sem.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/notification.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/mutex.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/synchronization/internal/graphcycles.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/hash/internal/hash.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/container/internal/hashtablez_sampler.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
INFO: From Compiling external/com_google_absl/absl/container/internal/hashtablez_sampler_force_weak_definition.cc:
cl : Command line warning D9025 : overriding '/w' with '/W3'
ERROR: C:/tensorflow-build/tensorflow/tensorflow/core/BUILD:2750:1: Executing genrule //tensorflow/core:version_info_gen failed (Exit 2): bash.exe failed: error executing command
  cd C:/users/kelly duong/_bazel_kelly duong/j7bi4x5j/execroot/org_tensorflow
  SET PATH=C:\msys64\usr\bin;C:\msys64\bin;C:\condaconda64\envs\tensorflow-build;C:\condaconda64\envs\tensorflow-build\Library\mingw-w64\bin;C:\condaconda64\envs\tensorflow-build\Library\usr\bin;C:\condaconda64\envs\tensorflow-build\Library\bin;C:\condaconda64\envs\tensorflow-build\Scripts;C:\condaconda64\envs\tensorflow-build\bin;C:\condaconda64\condabin;C:\Program Files (x86)\Common Files\Oracle\Java\javapath;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0;C:\WINDOWS\System32\OpenSSH;C:\Program Files\Intel\WiFi\bin;C:\Program Files\Common Files\Intel\WirelessCommon;C:\Program Files\MATLAB\R2019a\bin;c:\Program Files (x86)\Intel\Intel(R) Management Engine Components\DAL;c:\Program Files\Intel\Intel(R) Management Engine Components\DAL;C:\Program Files\Git\cmd;C:\Program Files\PuTTY;C:\Users\Kelly Duong\AppData\Local\Programs\Python\Python38-32\Scripts;C:\Users\Kelly Duong\AppData\Local\Programs\Python\Python38-32;C:\Users\Kelly Duong\AppData\Local\Microsoft\WindowsApps;C:\Users\Kelly Duong\AppData\Local\Programs\Microsoft VS Code\bin;C:\Users\Kelly Duong\AppData\Local\Microsoft\WindowsApps;C:\Program Files (x86)\Nmap;C:\Users\Kelly Duong\AppData\Local\Google\Cloud SDK\google-cloud-sdk\bin;C:\msys64\usr\bin
    SET PYTHON_BIN_PATH=C:/condaconda64/envs/tensorflow-build/python.exe
    SET PYTHON_LIB_PATH=C:/condaconda64/envs/tensorflow-build/lib/site-packages
    SET TF_CONFIGURE_IOS=0
    SET TF_DOWNLOAD_CLANG=0
    SET TF_NEED_CUDA=0
    SET TF_NEED_OPENCL_SYCL=0
    SET TF_NEED_ROCM=0
  C:/msys64/usr/bin/bash.exe -c source external/bazel_tools/tools/genrule/genrule-setup.sh; bazel-out/x64_windows-opt/bin/tensorflow/tools/git/gen_git_source.exe --generate external/local_config_git/gen/spec.json external/local_config_git/gen/head external/local_config_git/gen/branch_ref "bazel-out/x64_windows-opt/genfiles/tensorflow/core/util/version_info.cc" --git_tag_override=${GIT_TAG_OVERRIDE:-}
Execution platform: @bazel_tools//platforms:host_platform
C:/condaconda64/envs/tensorflow-build/python.exe: can't open file 'C:\users\kelly': [Errno 2] No such file or directory
Target //tensorflow/tools/pip_package:build_pip_package failed to build
INFO: Elapsed time: 161.540s, Critical Path: 21.87s
INFO: 188 processes: 188 local.
FAILED: Build did NOT complete successfully


SOLUTION: checked issues in github, found someone who created a conversion through colab in Edje's github issues pages:
https://colab.research.google.com/drive/1DolVXkn1w0ADa4B_jAuCpjSnyRSApH7D?authuser=2#scrollTo=oBjfejA4FgTf
(if link can't be open, but opens in incognito, re-enable third-party cookie blocking by clicking the eye in the url to turn it back on)







