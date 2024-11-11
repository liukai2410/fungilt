# fungilt
a deep learning framework for robust classification of fungal species from ITS sequences

![abstract](./images/fig0.png)

Note: The data and code are incomplete, and some content will be uploaded after the paper is included.

## 1 Environment configuration

### 1.1 Environment dependent files

You can choose to clone this project in its entirety

```python
git clone https://github.com/liukai2410/fungilt.git
```
Or download the dependent files separately: requirements.txt

### 1.2 Create a new conda environment
For ease of management and to prevent conflicts with the existing environment, we suggest creating a new conda environment
```python
conda create -n your_env_name python=3.8
conda activate your_env_name
```

### 1.3 Install all related dependencies
```python
# Ensure that it is currently located at the location of the dependent file
python -m pip install -r requirements.txt
```

## 2 Installation and project deployment
### 2.1 Project deployment
We recommend cloning this project directly to your local computer for faster project deployment
```python
  git clone https://github.com/liukai2410/fungilt.git
```
However, in fact, the project may contain a large number of files that are not useful to you, which could result in significant traffic loss. If you are confident enough, we also recommend that you download the corresponding files according to your needs to complete the deployment of the project.
### 2.2 File tree
```python
·
│──config/
│  └──config.yaml
│──datasets/
│──src/
│  │──main.py
│  │──util.py  # Settings and feature selection
│  │──train/
│  │  │──train.py
│  │  └──test.py
│  │──save_models/  # The model we trained
│  │──models/
│  │  │──utils.py
│  │  └──rnn_models.py
│  │——feature_extraction/
│  │  │——labels_encoding.py
│  │  └──sequence_encoding.py
│  └──danbert_models/ 
│     │——dnabert-3
│     │——dnabert-4
│     │——dnabert-5
│     │——dnabert-6
│     └──dnabert2
│——embeddings/  # Intermediate encoding files for data processing
│——results/
│  └──tensorboard/  # Draw a learning curve
└──output.log  # Record the running status of the program
```
## 3 Quickstart
## 4 Instructions
### 4.1 Data preparation

### 4.2 Using pre trained models
We offer multiple models that have been trained on different fungal scales, and you can choose to use our pre trained models directly, which will greatly reduce training time and computational costs.

**Note**: But if you have your own dataset, we often recommend using our model structure for retraining and parameter tuning, which may yield better results on your dataset.

We provide most of the information that needs to be set up and configured in the config.yaml file.

If you want to use our model for direct prediction or just for validation, you can follow the steps below：

  1. Project configuration. Download the necessary files to configure your project according to your needs
    
     ```python
     config.yaml  # Contains most of the modifiable configuration information
     src/mian.py
     src/utils.py
     src/train/test.py  # Load the model and use it for prediction
     src/feature_extraction/  # Encode the sequence
     ```
  2. Data processing. Process the form of the dataset according to the example of our dataset
     
     ```python
     # 1 Process the dataset format based on our own dataset
     # 2 We provide methods for processing datasets, including files in the most commonly used fasta format
     data_processing.py
     ```
  3. Modify configuration information

     ```python
     # 1 Confirm that the training mode of the model has been turned off
     train_mode:
       enable : 0
     # 2 Confirm that the kmer, sequence encoding method, etc. meet the requirements of the specific model
     # 3 Confirm if the path using the dataset is correct
     ```
  4. Run

     ```python
     # 1 Enter your project file，
     cd your_project
     # 2 Activate your operating environment
     conda activate your_env_name
     # 3 Run, reload the running information to the log file
     nohup python ./src/main.py > ./src/output.log 2>&1
     ```

### 4.3 Retrain the model
If you have your own dataset, we recommend retraining the model. You can modify some parameters as needed to improve its performance on your dataset.

The steps to retrain the model are as follows：

  1. Project configuration.

     ```python
     config.yaml  # Contains most of the modifiable configuration information
     src/mian.py
     src/utils.py
     src/train/train.py  # training model
     src/models/  # Saved multiple model architectures
     src/feature_extraction/  # Encode the sequence
     src/danbert_models/  # Saved the pre trained model of dnabert for sequence encoding processing (if needed)
     
     ```
  2. Data processing.

      ```python
     # 1 Process the dataset format based on our own dataset
     # 2 We provide methods for processing datasets, including files in the most commonly used fasta format
     data_processing.py
     ```
  3. Modify configuration information

     ```python
     # 1 Ensure that the training mode of the model is enabled
     train_mode:
       enable : 1
     # 2 Confirm that the kmer, sequence encoding method, etc. meet the requirements of the specific model
     # 3 Confirm if the path using the dataset is correct
     # 4 Provides more flexible and diverse parameter settings, including the model architecture used, learning rate, loss function, and some mechanisms of the training process. You can make adjustments according to your own needs to train a personalized model that fits your dataset.
     ```
  4. Run

     ```python
     # 1 Enter your project file，
     cd your_project
     # 2 Activate your operating environment
     conda activate your_env_name
     # 3 Run, reload the running information to the log file
     nohup python ./src/main.py > ./src/output.log 2>&1
     ```
