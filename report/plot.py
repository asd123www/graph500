import numpy as np
import matplotlib.pyplot as plt


def two_bar_chart():
    N = 7

    valid = (50, 197446, 3938017, 469785, 1799, 3, 0)
    all_edge = (50, 355613, 162580558, 104895143, 594484, 1805, 3)
    # boyStd = (2, 3, 4, 1, 2)
    # girlStd = (3, 5, 2, 3, 3)
    ind = np.arange(N)  
    width = 0.35
    
    fig = plt.subplots(figsize =(10, 7))
    # p1 = plt.bar(ind, valid, width, yerr = boyStd)
    # p2 = plt.bar(ind, all_edge, width, bottom = valid, yerr = girlStd)
    p1 = plt.bar(ind, valid, width)
    p2 = plt.bar(ind, all_edge, width, bottom = valid)
    
    plt.ylabel('# of edge reference')
    plt.title('Valid edge reference percentage')
    plt.xticks(ind, ('stage 1', 'stage 2', 'stage 3', 'stage 4', 'stage 5', 'stage 6', 'stage 7'))
    plt.yticks(np.arange(0, 170000000, 10000000))
    plt.legend((p1[0], p2[0]), ('valid', 'total'))

    plt.savefig("example.jpg")
    # plt.show()


def bar_chart():
    # creating the dataset
    data = {'stage 1': 50, 
            'stage 2': 197446, 
            'stage 3': 3938017,
            'stage 4': 469785,
            'stage 5': 1799, 
            'stage 6': 3,
            'stage 7': 0,}
    courses = list(data.keys())
    values = list(data.values())
    
    fig = plt.figure(figsize = (10, 7))
    
    # creating the bar plot
    plt.bar(courses, values, width = 0.4)
    
    plt.xlabel("stage number")
    plt.ylabel("# of points")
    plt.title("Points in each stage")
    plt.savefig("example.jpg")

def bar_chart_topdown_time():
    data = {'stage 1': 0.001, 
            'stage 2': 0.007, 
            'stage 3': 0.780,
            'stage 4': 0.504,
            'stage 5': 0.006, 
            'stage 6': 0.000,
            'stage 7': 0.000,}
    courses = list(data.keys())
    values = list(data.values())
    
    fig = plt.figure(figsize = (10, 7))
    
    # creating the bar plot
    plt.bar(courses, values, width = 0.4)
    
    plt.xlabel("stage number")
    plt.ylabel("time (s)")
    plt.title("time in each stage")
    plt.savefig("example.jpg")




def bar_chart_bottomup_time():
    data = {'stage 1': 0.296, 
            'stage 2': 0.149, 
            'stage 3': 0.096,
            'stage 4': 0.044,
            'stage 5': 0.017, 
            'stage 6': 0.016,
            'stage 7': 0.016,}
    courses = list(data.keys())
    values = list(data.values())
    
    fig = plt.figure(figsize = (10, 7))
    
    # creating the bar plot
    plt.bar(courses, values, width = 0.4)
    
    plt.xlabel("stage number")
    plt.ylabel("time (s)")
    plt.title("time in each stage")
    plt.savefig("example.jpg")

def contrast_bar_graph():

    # set width of bar
    barWidth = 0.25
    fig = plt.subplots(figsize =(10, 7))
    
    # set height of bar
    TOP_down = [0.001, 0.007, 0.780, 0.504, 0.006, 0.000, 0.000]
    BOTTOM_UP = [0.296, 0.149, 0.096, 0.044, 0.017, 0.016, 0.016]
    # CSE = [29, 3, 24, 25, 17]
    
    # Set position of bar on X axis
    br1 = np.arange(len(TOP_down))
    br2 = [x + barWidth for x in br1]
    br3 = [x + barWidth for x in br2]
    
    # Make the plot
    plt.bar(br1, TOP_down, color ='r', width = barWidth,
            edgecolor ='grey', label ='top_down')
    plt.bar(br2, BOTTOM_UP, color ='g', width = barWidth,
            edgecolor ='grey', label ='bottom_up')
    # plt.bar(br3, CSE, color ='b', width = barWidth,
    #         edgecolor ='grey', label ='CSE')
    
    # Adding Xticks
    plt.xlabel('stage number', fontweight ='bold', fontsize = 15)
    plt.ylabel('time(s)', fontweight ='bold', fontsize = 15)
    plt.xticks([r + barWidth for r in range(len(TOP_down))],
            ['stage 1', 'stage 2', 'stage 3', 'stage 4', 'stage 5', 'stage 6', 'stage 7'])
    
    plt.legend()
    plt.savefig("example.jpg")


def line_graph_perf():
	# line 1 points
	# y1 = [0.120211, 0.123066, 0.12387, 0.123131, 0.120557, 0.12217, 0.121263, 0.123709, 0.121501, 0.123956, 
	# 	  0.121208, 0.122174, 0.0976722, 0.0982671, 0.098765, 0.0994819, 0.100456, 0.0983159, 0.0981087, 0.0966537,
	# 	  0.098393, 0.0982386, 0.0993823, 0.0981683, 0.0989255, 0.0966361, 0.100275, 0.0978873, 0.0990832, 0.0976444,
        #           0.0988363, 0.099667, 0.0990524, 0.0978348, 0.0997225, 0.098606, 0.0999726, 0.0978333, 0.0997043, 0.0964872,
        #           0.0988136, 0.0978403, 0.0985235, 0.0981828, 0.0990339, 0.09836, 0.0989555, 0.0982127, 0.0995104, 0.0985152]
	# x1 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
	# 	  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	# 	  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        #           31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        #           41, 42, 43, 44, 45, 46, 47, 48, 49, 50]
	# plotting the line 1 points 

	x1 = [21, 22, 23, 24]
	y1 = [0.34217, 0.710704, 1.50816, 2.82894]
	plt.plot(x1, y1, label = "top-down")

	# line 2 points
	x2 = [21, 22, 23, 24]
	y2 = [0.0651495, 0.164762, 0.36894, 0.782756]
	# plotting the line 2 points 
	plt.plot(x2, y2, label = "bottom-up")


	# line 2 points
	x3 = [21, 22, 23, 24]
	y3 = [0.0195314, 0.0399537, 0.076043, 0.154498]
	# plotting the line 2 points 
	plt.plot(x3, y3, label = "hybrid")

	# naming the x axis
	plt.xlabel('psize')
	# naming the y axis
	plt.ylabel('time(s)')
	# giving a title to my graph
	# plt.title('Two lines on same graph!')
	plt.title('performance contrast')

	# show a legend on the plot
	plt.legend()

	# function to show the plot
	plt.savefig("example.jpg")


def line_graph_scale():
	x1 = [1, 2, 4, 8]
	y1 = [4.26422, 4.05262, 2.5586, 1.36046]
	plt.plot(x1, y1, label = "top-down")

	# line 2 points
	x2 = [1, 2, 4, 8]
	y2 = [2.44518, 1.22023, 0.624221, 0.359567]
	# plotting the line 2 points 
	plt.plot(x2, y2, label = "bottom-up")


	# line 2 points
	x3 = [1, 2, 4, 8]
	y3 = [0.41975, 0.223568, 0.119508, 0.0752094]
	# plotting the line 2 points 
	plt.plot(x3, y3, label = "hybrid")

	# naming the x axis
	plt.xlabel('psize')
	# naming the y axis
	plt.ylabel('time(s)')
	# giving a title to my graph
	# plt.title('Two lines on same graph!')
	plt.title('scalability')

	# show a legend on the plot
	plt.legend()

	# function to show the plot
	plt.savefig("example.jpg")

if __name__ == "__main__":
    line_graph_scale()
