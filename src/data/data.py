from torch.utils.data import Dataset
import torch
from sklearn.model_selection import train_test_split
from utils import *

"""
文件说明    该文件中是数据集的构建相关方法

"""


class MyDataset(Dataset):
    
    def __init__(self, data, labels):
        self.data = data
        self.labels = labels

    def __len__(self):
        return len(self.data)
    
    def __getitem__(self, index):
        return self.data[index], self.labels[index]
    
"""
装载数据并分割数据集
"""   
def get_mydataset(data, labels, conv_dim):
    
    print(f"==== 开始加载与整理数据集......")
    config = load_config()
    
    # 转换为张量数据
    tensor_data = torch.tensor(data, dtype=torch.float32)
    tensor_labels = torch.tensor(labels, dtype=torch.float32)
    
    # 修改数据形状
    if conv_dim == 1:
        if config['seq_encoder']['name'] == 'word2vec2':
            tensor_data = tensor_data
            print(tensor_data.shape)
        # elif config['seq_encoder']['name'] == 'dnabert2':
        #     tensor_data = tensor_data.squeeze(1)
        #     print(tensor_data.shape)
        else:
            tensor_data = tensor_data.unsqueeze(1)
    
    if conv_dim == 2:
        tensor_data = tensor_data.unsqueeze(1)
        tensor_data = tensor_data.unsqueeze(1)

    # 分割数据集
    # 这是随机分割的，平衡数据集分割的结果也平衡，但是如果不平衡数据集分割结果可能也不平衡，可以使用StratifiedShuffleSplit
    train_data,test_data,train_labels,test_labels = train_test_split(tensor_data, tensor_labels, test_size=0.2,random_state=42)
    val_data,test_data,val_labels,test_labels = train_test_split(test_data, test_labels, test_size=0.5,random_state=42)
    
    train_dataset = MyDataset(train_data, train_labels)
    val_dataset = MyDataset(val_data, val_labels)
    test_dataset = MyDataset(test_data, test_labels)
    
    print(f"==== 数据集加载与整理完成")
    print(f"==== 训练集长度：{len(train_data)}")
    print(f"==== 验证集长度：{len(val_data)}")
    print(f"==== 测试集长度：{len(test_data)}")
    print(f"==== 数据形状{test_data[0].shape}")
    print(f"==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== =====")
    
    return train_dataset, val_dataset, test_dataset

# 分割数据集，要开启交叉验证，只把数据集分成训练集和测试集
def get_mydataset_kfold(data, labels, conv_dim):
    
    print(f"==== 开始加载与整理数据集......")
    config = load_config()
    
    # 转换为张量数据
    tensor_data = torch.tensor(data, dtype=torch.float32)
    tensor_labels = torch.tensor(labels, dtype=torch.float32)
    
    # 修改数据形状
    if conv_dim == 1:
        if config['seq_encoder']['name'] == 'word2vec2':
            tensor_data = tensor_data
        # elif config['seq_encoder']['name'] == 'dnabert2':
        #     tensor_data = tensor_data.squeeze(1)
        #     print(tensor_data.shape)
        else:
            tensor_data = tensor_data.unsqueeze(1)
    
    if conv_dim == 2:
        tensor_data = tensor_data.unsqueeze(1)
        tensor_data = tensor_data.unsqueeze(1)

    # 分割数据集
    # 这是随机分割的，平衡数据集分割的结果也平衡，但是如果不平衡数据集分割结果可能也不平衡，可以使用StratifiedShuffleSplit
    train_data,test_data,train_labels,test_labels = train_test_split(tensor_data, tensor_labels, test_size=0.1,random_state=42)
    # val_data,test_data,val_labels,test_labels = train_test_split(test_data, test_labels, test_size=0.5,random_state=42)
    
    train_dataset = MyDataset(train_data, train_labels)
    # val_dataset = MyDataset(val_data, val_labels)
    test_dataset = MyDataset(test_data, test_labels)
    
    print(f"==== 数据集加载与整理完成")
    print(f"==== 训练集长度：{len(train_data)}")
    # print(f"==== 验证集长度：{len(val_data)}")
    print(f"==== 测试集长度：{len(test_data)}")
    print(f"==== 数据形状{test_data[0].shape}")
    print(f"==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== =====")
    
    return train_dataset, test_dataset


# 两组数据输入
class MyDataset1(Dataset):
    
    def __init__(self, data1, data2, labels):
        self.data1 = data1
        self.data2 = data2
        self.labels = labels

    def __len__(self):
        return len(self.labels)
    
    def __getitem__(self, index):
        return (self.data1[index], self.data2[index]), self.labels[index]

# 三组数据输入
class MyDataset3(Dataset):
    
    def __init__(self, data1, data2, data3, labels):
        self.data1 = data1
        self.data2 = data2
        self.data3 = data3
        self.labels = labels

    def __len__(self):
        return len(self.labels)
    
    def __getitem__(self, index):
        return (self.data1[index], self.data2[index], self.data3[index]), self.labels[index]
      
"""
装载数据并分割数据集 两组数据并行
"""   
def get_mydataset1(data1, data2, labels, conv_dim):
    
    print(f"==== 开始加载与整理数据集......")
    config = load_config()
    
    # 转换为张量数据
    tensor_data1 = torch.tensor(data1, dtype=torch.float32)
    tensor_data2 = torch.tensor(data2, dtype=torch.float32)
    tensor_labels = torch.tensor(labels, dtype=torch.float32)
    
    # 修改数据形状
    if conv_dim == 1:
        if config['seq_encoder']['name'] == 'word2vec2':
            tensor_data1 = tensor_data1
            tensor_data2 = tensor_data2
        else:
            tensor_data1 = tensor_data1.unsqueeze(1)
            tensor_data2 = tensor_data2.unsqueeze(1)
    
    if conv_dim == 2:
        tensor_data1 = tensor_data1.unsqueeze(1)
        tensor_data1 = tensor_data1.unsqueeze(1)
        tensor_data2 = tensor_data2.unsqueeze(1)
        tensor_data2 = tensor_data2.unsqueeze(1)

    # 断言，两组数据长度应该一致
    assert len(tensor_data1) == len(tensor_data2)
    
    # 合并
    combined_data = list(zip(tensor_data1, tensor_data2, tensor_labels))
    
    # 分割数据集
    # 这是随机分割的，平衡数据集分割的结果也平衡，但是如果不平衡数据集分割结果可能也不平衡，可以使用StratifiedShuffleSplit
    train_combined,test_combined = train_test_split(combined_data, test_size=0.2,random_state=42)
    val_combined,test_combined = train_test_split(test_combined, test_size=0.5,random_state=42)
    
    train_data1, train_data2, train_labels = zip(*train_combined)
    test_data1, test_data2, test_labels = zip(*test_combined)
    val_data1, val_data2, val_labels = zip(*val_combined)
    
    train_dataset = MyDataset1(train_data1, train_data2, train_labels)
    val_dataset = MyDataset1(val_data1, val_data2, val_labels)
    test_dataset = MyDataset1(test_data1, test_data2, test_labels)
    
    print(f"==== 数据集加载与整理完成")
    print(f"==== 训练集长度：data1:{len(train_data1)} == data2:{len(train_data2)}")
    print(f"==== 验证集长度：{len(val_data1)}")
    print(f"==== 测试集长度：{len(test_data1)}")
    print(f"==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== =====")
    
    return train_dataset, val_dataset, test_dataset

"""
装载数据并分割数据集 两组数据并行
"""   
def get_mydataset3(data1, data2, data3, labels, conv_dim):
    
    print(f"==== 开始加载与整理数据集......")
    config = load_config()
    
    # 转换为张量数据
    tensor_data1 = torch.tensor(data1, dtype=torch.float32)
    tensor_data2 = torch.tensor(data2, dtype=torch.float32)
    tensor_data3 = torch.tensor(data3, dtype=torch.float32)
    tensor_labels = torch.tensor(labels, dtype=torch.float32)
    
    # 修改数据形状
    if conv_dim == 1:
        if config['seq_encoder']['name'] == 'word2vec2':
            tensor_data1 = tensor_data1
            tensor_data2 = tensor_data2
            tensor_data3 = tensor_data3
        else:
            tensor_data1 = tensor_data1.unsqueeze(1)
            tensor_data2 = tensor_data2.unsqueeze(1)
            tensor_data3 = tensor_data3.unsqueeze(1)
    
    if conv_dim == 2:
        tensor_data1 = tensor_data1.unsqueeze(1)
        tensor_data1 = tensor_data1.unsqueeze(1)
        tensor_data2 = tensor_data2.unsqueeze(1)
        tensor_data2 = tensor_data2.unsqueeze(1)
        tensor_data3 = tensor_data3.unsqueeze(1)
        tensor_data3 = tensor_data3.unsqueeze(1)

    # 断言，两组数据长度应该一致
    assert len(tensor_data1) == len(tensor_data2) == len(tensor_data3)
    
    # 合并
    combined_data = list(zip(tensor_data1, tensor_data2, tensor_data3, tensor_labels))
    
    # 分割数据集
    # 这是随机分割的，平衡数据集分割的结果也平衡，但是如果不平衡数据集分割结果可能也不平衡，可以使用StratifiedShuffleSplit
    train_combined,test_combined = train_test_split(combined_data, test_size=0.2,random_state=42)
    val_combined,test_combined = train_test_split(test_combined, test_size=0.5,random_state=42)
    
    train_data1, train_data2, train_data3, train_labels = zip(*train_combined)
    test_data1, test_data2, test_data3, test_labels = zip(*test_combined)
    val_data1, val_data2, val_data3, val_labels = zip(*val_combined)
    
    train_dataset = MyDataset3(train_data1, train_data2, train_data3, train_labels)
    val_dataset = MyDataset3(val_data1, val_data2, val_data3, val_labels)
    test_dataset = MyDataset3(test_data1, test_data2, test_data3, test_labels)
    
    print(f"==== 数据集加载与整理完成")
    print(f"==== 训练集长度：data1:{len(train_data1)} == data2:{len(train_data2)}")
    print(f"==== 验证集长度：{len(val_data1)}")
    print(f"==== 测试集长度：{len(test_data1)}")
    print(f"==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== =====")
    
    return train_dataset, val_dataset, test_dataset

# 融合视角的交叉验证
def get_mydataset_kfold2(data1, data2, labels, conv_dim):
    
    print(f"==== 开始加载与整理数据集......")
    config = load_config()
    
    # 转换为张量数据
    tensor_data1 = torch.tensor(data1, dtype=torch.float32)
    tensor_data2 = torch.tensor(data2, dtype=torch.float32)
    tensor_labels = torch.tensor(labels, dtype=torch.float32)
    
    # 修改数据形状
    if conv_dim == 1:
        if config['seq_encoder']['name'] == 'word2vec2':
            tensor_data1 = tensor_data1
            tensor_data2 = tensor_data2
        # elif config['seq_encoder']['name'] == 'dnabert2':
        #     tensor_data = tensor_data.squeeze(1)
        #     print(tensor_data.shape)
        else:
            tensor_data1 = tensor_data1.unsqueeze(1)
            tensor_data2 = tensor_data2.unsqueeze(1)
    
    if conv_dim == 2:
        tensor_data1 = tensor_data1.unsqueeze(1)
        tensor_data1 = tensor_data1.unsqueeze(1)
        tensor_data2 = tensor_data2.unsqueeze(1)
        tensor_data2 = tensor_data2.unsqueeze(1)

    # 断言，两组数据长度应该一致
    assert len(tensor_data1) == len(tensor_data2)
    
    # 合并
    combined_data = list(zip(tensor_data1, tensor_data2, tensor_labels))
    
    # 分割数据集
    # 这是随机分割的，平衡数据集分割的结果也平衡，但是如果不平衡数据集分割结果可能也不平衡，可以使用StratifiedShuffleSplit
    train_combined,test_combined = train_test_split(combined_data, test_size=0.1,random_state=42)
    # val_data,test_data,val_labels,test_labels = train_test_split(test_data, test_labels, test_size=0.5,random_state=42)
    
    train_data1, train_data2, train_labels = zip(*train_combined)
    test_data1, test_data2, test_labels = zip(*test_combined)
    
    train_dataset = MyDataset1(train_data1, train_data2, train_labels)
    # val_dataset = MyDataset(val_data, val_labels)
    test_dataset = MyDataset1(test_data1, test_data2, test_labels)
    
    print(f"==== 数据集加载与整理完成")
    print(f"==== 训练集长度：{len(train_data1)}")
    # print(f"==== 验证集长度：{len(val_data)}")
    print(f"==== 测试集长度：{len(test_data1)}")
    print(f"==== 数据形状{test_data1[0].shape}")
    print(f"==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== =====")
    
    return train_dataset, test_dataset
