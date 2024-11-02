# fungilt
a deep learning framework for robust classification of fungal species from ITS sequences

![abstract](./images/fig0.png)

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

### 2.2 File tree

### 2.3 Necessary document description


## 3 Quickstart
### 3.1 Data preparation

### 3.2 Using pre trained models
We offer multiple models that have been trained on different fungal scales, and you can choose to use our pre trained models directly, which will greatly reduce training time and computational costs.

**Note**: But if you have your own dataset, we often recommend using our model structure for retraining and parameter tuning, which may yield better results on your dataset.
### 3.3 Retrain the model
