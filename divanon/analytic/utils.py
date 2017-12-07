import numpy as np
from random import random

def normalize_np_array(np_array):
    array_max = np.amax(np_array)
    if (array_max > 0):
        np_array = np_array / array_max
    return np_array

def randomly_split_list(in_list, first_part_len):
    first_part_len = int(first_part_len)
    list1 = []
    list2 = []
    first_part_probability = first_part_len / len(in_list)
    # firstly make a non-precise distribution
    for item in in_list:
        dice_roll = random()
        if dice_roll < first_part_probability:
            list1.append(item)
        else:
            list2.append(item)
    # now ensure the requested ratio
    return balance_lists(list1, list2, first_part_len)

def balance_lists(list1, list2, first_part_len):
    difference = first_part_len - len(list1)
    if difference > 0:
        list1 += list2[-difference:]
        list2 = list2[:-difference]
    else:
        list2 += list1[difference:]
        list1 = list1[:difference]
    return ( list1, list2 )
