import yaml
import numpy as np
import torch
import random
import time 
from datetime import datetime
import logging
from feature_extraction.sequence_encoding import *
from feature_extraction.labels_encoding import *


"""
Read configuration file
"""
def load_config():
    config_file = "./configs/configs.yaml"
    with open(config_file, "r") as f:
        config = yaml.safe_load(f)
    return config

"""
Set random seeds
"""
def set_seed():
    config = load_config()
    seed_size = config["seed"]["seed_size"]
    random.seed(seed_size)
    np.random.seed(seed_size)
    torch.manual_seed(seed_size)
   
    
"""
Set GPU environment
"""
def get_accelerated_computing_devices():
    
    # Get configuration information
    config = load_config()
    # All available GPUs in the server
    gpu_info = ["GPU0:3080Ti", "GPU1:3080Ti", "GPU2:4090", "GPU3:4090"]
    # Determine the available GPUs based on the configuration file
    gpu_ids = config['gpu']['gpu_ids']
    if len(gpu_ids) == 1:
        l = 29
    if len(gpu_ids) == 2:
        l = 26
    if len(gpu_ids) == 3:
        l = 23
    if len(gpu_ids) == 4:
        l = 20
    if torch.cuda.is_available():
        # Is the GPU device available? If so, print out the available GPU information
        device_str = "cuda:" + str(gpu_ids[0])
        device = torch.device(device_str)
        
        print(f"==== GPU设备可用: {gpu_ids}{'':<{l}} ==== ==== ==== ====")
        print(f"===={'':<62}====")
        for id in gpu_ids:
            print(f"==== {id} - {gpu_info[id]:<57}====")
        print(f"===={'':<62}====")   
        print(f"==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ===== ")
    else:
        print("==== ==== ==== No GPU acceleration devices available ==== ==== ====")
        device = torch.device("cpu")
    
    print(gpu_ids)
    print(gpu_ids[0])
    print(device)
    return gpu_ids, device
    

"""
Function：read_current_time()
Function function: Read the current time
"""
def read_current_time(time):
    
    # Get the current timestamp
    timestamp = time
    # Convert timestamp to timestamp object
    dt_object = datetime.fromtimestamp(timestamp)
    # Convert the timestamp object to a human readable string format
    human_readable_time = dt_object.strftime('%Y-%m-%d %H:%M:%S')
    
    return human_readable_time


"""
Select loss function
    Determine which loss function to choose and define it in the config configuration file
    
"""
def select_loss_function():
    config = load_config()
    # Select loss function
    # Cross entropy loss function
    if config['train']['criterion'] == 'crossentropyloss':
        criterion = torch.nn.CrossEntropyLoss()
    
    return criterion

"""
Select optimizer
    Determine which optimizer to choose and define it in the config configuration file
    
"""
def select_optimizer_function(model):
    config = load_config()
    # Select loss function
    # Cross entropy loss function
    if config['train']['optimizer'] == 'adam':
        lr = config['train']['lr']
        
        # optimizer = torch.optim.Adam(model.parameters(), lr=lr)
        # Add regularization
        optimizer = torch.optim.Adam(model.parameters(), lr=lr, weight_decay=1e-5)
        
    
    return optimizer



"""
Obtain data path
"""
def select_file_path():
    config = load_config()
    file_names = [
        "test",
        "count_sequences_test_500",
        "count_sequences_test_400",
        "count_sequences_test_300",
        "count_sequences_test_200",
        "count_sequences_test_100",
        "count_sequences_test_50",
        "count_sequences_test_20",
        "01_dataset",
        "02_dataset",
        "03_dataset",
        "04_dataset",
        "04_dataset_s",
        "05_dataset",
        "06_dataset",
        "07_dataset",
        "08_dataset",
        "09_dataset",
        "10_dataset",
        "11_dataset",
        
    ]
    
    for file in config['file_path']:
        if file['name'] in file_names and file['value']['enable'] == 1:
            return file['value']['path']
    
    return None  # If there is no file path that meets the criteria, return None


"""
Create a logger recorder
"""
def create_logger(logs='./src/logs',handler='file',file_name='train.txt'):
    # 01 Generate Recorder
    logger = logging.getLogger(logs)
    # 02 Set Recorder Level
    logger.setLevel(logging.DEBUG)
    # 03 Where to write console/files to when creating processors
    if handler == 'file':
        # The writing method is overwrite
        consoleHandler = logging.FileHandler(filename=f"{logs}/{file_name}", mode='w')
    elif handler == 'stream':
        consoleHandler = logging.StreamHandler()
    # 04 Set processor level
    consoleHandler.setLevel(logging.DEBUG)
    # 05 Set the Reformer format
    formatter = logging.Formatter("%(asctime)s|%(message)s")
    # 06 Associate processors and formats
    consoleHandler.setFormatter(formatter)
    # 07 Associate recorders and processors
    logger.addHandler(consoleHandler)
    
    return logger

"""
Select the encoding method for the sequence and then encode the sequence accordingly
"""
def select_sequences_encoder(seqs_kmer):
    
    # Get configuration file
    config = load_config()
    # Obtain encoding method
    encoder_name = config['seq_encoder']['name']
    k_size = config["k_mer"]["k"]
    
    if encoder_name == 'kmer_freq':
        data1 = calculate_kmer_frequency(seqs_kmer)
        return data1,None
    elif encoder_name == 'word2vec1':
        w2v_model = create_Word2Vec(seqs_kmer)
        vec_size = config['word2vec']['vector_size']
        data1 = create_word2vec_embedding(w2v_model, seqs_kmer, vec_size)
        return data1,None
    elif encoder_name == 'dna2vec':
        # Load pre trained model
        dna2vec_model_path = './src/dna2vec_model/dna2vec-20161219-0153-k3to8-100d-10c-29320Mbp-sliding-Xat.w2v'
        dna2vec_model = load_DNA2Vec_model(dna2vec_model_path)
        vec_size = 100
        data1 = create_dna2vec_embedding(dna2vec_model, seqs_kmer, vec_size)
        return data1,None
    elif encoder_name == 'rna2vec':
        # Load pre trained model
        dna2vec_model_path = './src/dna2vec_model/dna2vec-20161219-0153-k3to8-100d-10c-29320Mbp-sliding-Xat.w2v'
        dna2vec_model = load_DNA2Vec_model(dna2vec_model_path)
        vec_size = 100
        data1 = create_rna2vec_embedding(dna2vec_model, seqs_kmer, vec_size)
        return data1,None
    elif encoder_name == 'word2vec2':
        w2v_model = create_Word2Vec(seqs_kmer)
        vec_size = config['word2vec']['vector_size']
        # Sequence filling and cropping
        # kmer_sequences = sequences_padding_and_truncation(seqs_kmer)
        data1 = create_word2vec_embedding2(w2v_model, seqs_kmer, vec_size)
        return data1,None
    elif encoder_name == 'kmer_fre_and_word2vec1':
        data1 = calculate_kmer_frequency(seqs_kmer)
        w2v_model = create_Word2Vec(seqs_kmer)
        vec_size = config['word2vec']['vector_size']
        data2 = create_word2vec_embedding(w2v_model, seqs_kmer, vec_size)
        return data1, data2
    elif encoder_name == 'kmer_fre_and_dnabert':
        data1 = calculate_kmer_frequency(seqs_kmer)
        data2 = get_dnabert_embedding(seqs_kmer)
        return data1, data2
    elif encoder_name == 'kmer_fre_and_dnabert2':
        data1 = calculate_kmer_frequency(seqs_kmer)
        data2 = get_dnabert2_embedding(seqs_kmer)
        return data1, data2
    elif encoder_name == 'word2vec_and_dnabert':
        w2v_model = create_Word2Vec(seqs_kmer)
        vec_size = config['word2vec']['vector_size']
        data1 = create_word2vec_embedding(w2v_model, seqs_kmer, vec_size)
        data2 = get_dnabert_embedding(seqs_kmer)
        return data1, data2
    elif encoder_name == 'kmer_fre_and_dna2vec':
        data1 = calculate_kmer_frequency(seqs_kmer)
        dna2vec_model_path = './src/dna2vec_model/dna2vec-20161219-0153-k3to8-100d-10c-29320Mbp-sliding-Xat.w2v'
        dna2vec_model = load_DNA2Vec_model(dna2vec_model_path)
        vec_size = 100
        data2 = create_dna2vec_embedding(dna2vec_model, seqs_kmer, vec_size)
        return data1, data2
    elif encoder_name == 'dnabert':
        data1 = get_dnabert_embedding(seqs_kmer)
        return data1,None
    elif encoder_name == 'dnabert2':
        data1 = get_dnabert2_embedding(seqs_kmer)
        return data1,None
    elif encoder_name == 'kmer_freq_X': # Advanced version for calculating kmers frequency
        # data1 = calculate_kmer_frequency2(seqs_kmer, k_size)
        data1 = calculate_kmer_frequency2(seqs_kmer, k_size)
        return data1,None
    elif encoder_name == 'kmer_freX_and_word2vec1': # Advanced version for calculating kmers frequency
        # data1 = calculate_kmer_frequency2(seqs_kmer, k_size)
        data1 = calculate_kmer_frequency2(seqs_kmer, k_size)
        w2v_model = create_Word2Vec(seqs_kmer)
        vec_size = config['word2vec']['vector_size']
        data2 = create_word2vec_embedding(w2v_model, seqs_kmer, vec_size)
        return data1, data2

    else:
        raise ValueError("Unsupported encoder name") 

"""
Select the encoding method for the sequence and then encode the sequence accordingly
"""
def select_sequences_encoder2(seqs_kmer):
    
    # Get configuration file
    config = load_config()
    # Obtain encoding method
    encoder_name = config['seq_encoder']['name']
    k_size = config["k_mer"]["k"]
    
    if encoder_name == 'kmers_encoder1':
        data1 = calculate_kmer_frequency(seqs_kmer)
        w2v_model = create_Word2Vec(seqs_kmer)
        vec_size = config['word2vec']['vector_size']
        data2 = create_word2vec_embedding(w2v_model, seqs_kmer, vec_size)
        data3 = get_dnabert_embedding(seqs_kmer)
        return data1,data2, data3
    
    else:
        raise ValueError("Unsupported encoder name") 

"""
Select the encoding method for the label
"""
def select_labels_encoder(labels):
    config = load_config()
    encoder = config['label_encoder']['name'] 
    if encoder == 'int':
        labels = convert_to_int_labels(labels)
    if encoder == 'onehot':
        labels = create_one_hot_encoding(labels)
    
    return labels

    

def main():
    pass

if __name__ == '__main__':
    main()
