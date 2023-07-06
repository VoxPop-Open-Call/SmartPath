import csv
import mysql.connector
import pandas as pd
import os
import calendar
import json
import sys
import gc
import signal

from dotenv import load_dotenv
from mysql.connector import Error
from urllib.request import urlopen
from io import BytesIO
from zipfile import ZipFile


def handler(signum, frame):
	global conn

	print("Received SIGINT from user, terminating program")
	# Terminate DB connection
	conn.close()
	quit()
    
def DBConnection():
	# Get environment variables
	load_dotenv()
	hostname = os.getenv("hostname")
	DBName = os.getenv("dbnameSecure")
	username = os.getenv("usernameSecure")
	password = os.getenv("passwordSecure")
	port = int(os.getenv("portSecure"))

	# Try to create a connection
	try:
			conn = mysql.connector.connect(host = hostname, user = username, password = password, database = DBName, port = port)
			print("DB connection successful")
	except Error as error:
			print(f"Error while setting the DB connection -> '{error}'")

	return conn

def downloadZip(extractPath):
	url = "https://github.com/carrismetropolitana/gtfs/raw/live/CarrisMetropolitana.zip"

	httpResponse = urlopen(url)
	zipfile = ZipFile(BytesIO(httpResponse.read()))
	zipfile.extractall(path = extractPath)

def clearTable(tableName, conn):
	query = f"DELETE FROM {tableName}"
	try:
		cursor = conn.cursor()
		cursor.execute(query)
		conn.commit()
		print(f"Cleared table {tableName}")
		cursor.close()
	except Error as error:
		print(f"Error while clearing table {tableName} -> '{error}'")
		return False
	
	return True

def setBusStops(extractPath, conn, clearedBusStops):
	if not clearedBusStops:
		print("Verifying and setting table BusStops")
	else:
		print("Setting table BusStops")

	# Read stops file
	stopsPath = extractPath + "stops.txt"
	with open(stopsPath, 'r', encoding = "utf8") as file:
		lines = list(csv.reader(file, delimiter = ','))

	# * Line format is
	# stop_id,stop_code,stop_name,stop_lat,stop_lon,municipality,locality,near_health_clinic,near_hospital,near_university,near_school,near_police_station,near_fire_station,near_shopping,near_historic_building,near_transit_office,light_rail,subway,train,boat,airport,bike_sharing,bike_parking,car_parking

	stop_idArray = []
	stop_nameArray = []
	stop_latArray = []
	stop_lonArray = []

	# Ignore header line and get necessary information
	for row in lines[1:]:
		stop_idArray.append(int(row[0]))
		stop_nameArray.append(row[2])
		stop_latArray.append(float(row[3]))
		stop_lonArray.append(float(row[4]))

	stops = []
	for iter in range(len(stop_idArray)):
		tempStop = []
		tempStop.append(stop_idArray[iter])
		tempStop.append(stop_nameArray[iter])
		tempStop.append(stop_latArray[iter])
		tempStop.append(stop_lonArray[iter])

		if not clearedBusStops:
			# Verify if this entry exists in the DB and if it's valid
			query = "SELECT * FROM BusStops WHERE StopID = %s"
			
			try:
				cursor = conn.cursor()
				cursor.execute(query, tempStop[0])
				reply = cursor.fetchone()
				cursor.close()
			except Error as error:
				print(f"Error while verifying values for table BusStops on row with StopID {tempStop[0]} -> '{error}'")
				return False

			if(reply == None):	# Row is not present in the DB, add it
				stops.append(tempStop)
				#print(f"Adding row StopID = {tempStop[0]}, StopName = {tempStop[1]}, Latitude = {tempStop[2]}, Longitude = {tempStop[3]}, Type = {tempStop[4]}")
			else:								# Else, check if it matches the current information
				if(tempStop[1] != reply[1] or tempStop[2] != reply[2] or tempStop[3] != reply[3]):
					# Delete row in DB and insert current one
					query = "DELETE FROM BusStops WHERE StopID = %s"

					try:
						cursor = conn.cursor()
						cursor.execute(query, tempStop[0])
						conn.commit()
						cursor.close()
					except Error as error:
						print(f"Error while deleting row with StopID {tempStop[0]} from table BusStops -> '{error}'")
						return False

					stops.append(tempStop)
		else:
			stops.append(tempStop)
	
	# Ready query
	query = "INSERT INTO BusStops (StopID, StopName, Latitude, Longitude) VALUES (%s, %s, %s, %s)"

	# Execute query
	try:
		print("Creating query for table BusStops")
		cursor = conn.cursor()
		cursor.executemany(query, stops)
		conn.commit()
		print("Table BusStops set")
		cursor.close()
	except Error as error:
		print(f"Error while setting values for table BusStops -> '{error}'")
		return False
	
	return True

def setServiceModes(extractPath, conn, clearedServiceModes):
	if not clearedServiceModes:
		print("Verifying and setting table ServiceModes")
	else:
		print("Setting table ServiceModes")

	# Read calendar_dates file
	filePath = extractPath + "calendar_dates.txt"
	with open(filePath, 'r', encoding = "utf8") as file:
		lines = list(csv.reader(file, delimiter = ','))

	# * Line format is
	# service_id,date,holiday,period,day_type,exception_type

	# Get current year
	year = int(lines[1][1][:4])

	# Declare dictionary that will be used to input values to the DB
	dic = {}
	for month in range(1, 13):	# For each month, add a line for each day of it
		monthStr = f"{month:02}"
		_, daysInMonth = calendar.monthrange(year, month)
		for day in range(1, daysInMonth + 1):
			dayStr = f"{day:02}"
			
			# Add day to dictionary
			index = str(year) + monthStr + dayStr
			dic[f"{index}"] = ""

	# Check different services for a given day
	for day, _ in dic.items():
		for row in lines[1:]:
			if(row[1] == day):
				dic[f"{day}"] = dic.get(f"{day}") + row[0] + ','

	# Remove final ',' in each row
	for day, _ in dic.items():
		dic[f"{day}"] = dic.get(f"{day}")[:-1]

	queryRows = []
	for day, services in dic.items():
		queryRowTemp = []
		queryRowTemp.append(int(day))
		queryRowTemp.append(json.dumps(services))

		if not clearedServiceModes:
			# Verify if this entry exists in the DB and if it's valid
			query = "SELECT * FROM ServiceModes WHERE DayOfYear = %s"

			try:
				cursor = conn.cursor()
				cursor.execute(query, queryRowTemp[0])
				reply = cursor.fetchone()
				cursor.close()
			except Error as error:
				print(f"Error while verifying values for table ServicesModes on row with DayOfYear {queryRowTemp[0]} -> '{error}'")
				return False
			
			if(reply == None):	# Row is not present in the DB, add it
				queryRows.append(queryRowTemp)
			else:								# Else, check if it matches the current information
				if(queryRowTemp[1] != reply[1]):
					# Delete row in DB and insert current one
					query = "DELETE FROM BusStops WHERE StopID = %s"

					try:
						cursor = conn.cursor()
						cursor.execute(query, queryRowTemp[0])
						conn.commit()
						cursor.close()
					except Error as error:
						print(f"Error while deleting row with DayOfYear {queryRowTemp[0]} from table ServicesModes -> '{error}'")
						return False

					queryRows.append(queryRowTemp)
		else:
			queryRows.append(queryRowTemp)
	
	# Ready query
	query = "INSERT INTO ServiceModes (DayOfYear, Services) VALUES (%s, %s)"

	# Execute query
	try:
		print("Creating query for table ServiceModes")
		cursor = conn.cursor()
		cursor.executemany(query, queryRows)
		conn.commit()
		print("Table ServiceModes set")
		cursor.close()
	except Error as error:
		print(f"Error while setting values for table ServiceModes -> '{error}'")
		return False
	
	return True

def setBusTripsAndBusRoutes(extractPath, conn, clearedBusTrips, clearedBusRoutes):
	if not clearedBusTrips:
		print("Verifying and setting table BusTrips")
	else:
		print("Setting table BusTrips")

	if not clearedBusRoutes:
		print("Verifying and setting table BusRoutes")
	else:
		print("Setting table BusRoutes")
	print("This step may take a while")

	#* Read routes file
	#* Line format is
	# route_id,agency_id,route_short_name,route_long_name,route_type,route_color,route_text_color,circular,path_type
	filePath = extractPath + "routes.txt"
	with open(filePath, 'r', encoding = "utf8") as file:
		linesRoutes = list(csv.reader(file, delimiter = ','))

	# Get routes
	routes = []
	for row in linesRoutes[1:]:
		routes.append(row[0])
	del linesRoutes	# To avoid memory problems
	gc.collect()

	#* Read stop_times file
	#* Line format is
	# trip_id,arrival_time,departure_time,stop_id,stop_sequence,shape_dist_traveled,pickup_type,drop_off_type
	filePath = extractPath + "stop_times.txt"
	with open(filePath, 'r', encoding = "utf8") as file:
		linesStopTimes = list(csv.reader(file, delimiter = ','))

	#* Read trips file
	#* Line format is
	# route_id,service_id,trip_id,trip_headsign,direction_id,shape_id,calendar_desc
	filePath = extractPath + "trips.txt"
	with open(filePath, 'r', encoding = "utf8") as file:
		linesTrips = list(csv.reader(file, delimiter = ','))


	# These will speed up the program
	tripsPosition = 1 		
	stopTimesRoutesPosition = 1	
	stopTimesTimePosition = 1

	# This is to take care of out-of-order trips that may arise
	dicStops = {}	# key = str(RouteID) + str(Direction)
	dicTime = {}	# key = TripID

	skipped = False
	skippedTrips = []

	#? This huge for loop will go through every route and every line on trips.txt
	#? It will first populate the table BusRoutes with the stops of each route and then it will populate the table BusTrips with all the trips
	for route in routes:
		#print("-> Checking route", route)

		stopsAdded = False
		matched = False

		#! This step assumes that trips of the same route are grouped together, which seems to be the case
		# First check skipped trips TODO


		for rowTrips in linesTrips[tripsPosition:]:
			if(rowTrips[0] == route):  # Match
				matched = True
				trip = rowTrips[2]
				direction = rowTrips[4]

				#? Routes in one way may have different stops when compared to the same route in the other way (one way streets and such)
				
				# Check to see if stops were already added
				if(stopsAdded == False):
					#* First check if any of the routes there were out of order is the required one
					hadItSaved = False
					index = str(route) + str(direction)
					for savedIndex, savedStops in dicStops.items():
						if(savedIndex == index):	# Match
							stops = savedStops
							hadItSaved = True
							print("Had stops of route", route, "saved in dictionary")
							del dicStops[f"{index}"]
							break

					if not hadItSaved:
						# Go to stop_times and retrieve stops
						stops = []

						# Append direction of route
						#stops.append(direction)

						stopsRetrieved = False
						#? Order of RouteID is not the same in routes.txt and in stop_times.txt
						for rowStopTimes in linesStopTimes[stopTimesRoutesPosition:]:	
							if(rowStopTimes[0] == trip):
								stops.append(int(rowStopTimes[3]))
								stopsRetrieved = True
							elif stopsRetrieved:	# Already added, skip route
								break
							else:
								#print("Tried to find", trip, "and found", rowStopTimes[0])
								#* Order broke, save next stops on dictionary
								# Since the program doesn't know to which route the found trip relates to, it needs to do some backtracking
								tripFound = rowStopTimes[0]
								for rowTripsBacktrack in linesTrips[1:]:
									if(rowTripsBacktrack[2] == tripFound):	# Found a match
										routeBacktrack = rowTripsBacktrack[0]
										directionBacktrack = rowTripsBacktrack[4]
										break

								index = str(routeBacktrack) + str(directionBacktrack)
								# Create new entry in dictionary if it doesn't exist
								if(index in dicStops.keys()):
									alreadyAddedStops = dicStops.get(f"{index}")
									found = False
									for stop in alreadyAddedStops:
										if(stop == int(rowStopTimes[3])):
											found = True
											break

									# The first one can appear again in the end (only once)
									if(not found or (stop == alreadyAddedStops[0] and int(rowStopTimes[4]) != 1 and alreadyAddedStops[-1] != stop)):
										alreadyAddedStops.append(int(rowStopTimes[3]))
										dicStops[f"{index}"] = alreadyAddedStops
										#print("Added stop", int(rowStopTimes[3]), "to dicStops in entry", index)
								else:
									alreadyAddedStops = []
									alreadyAddedStops.append(int(rowStopTimes[3]))
									dicStops[f"{index}"] = alreadyAddedStops
									#print("Created entry", index, "in dicStops, and added stop", int(rowStopTimes[3]))

							stopTimesRoutesPosition += 1
					stopsAdded = True

					#? Inserting information like this on the DB will reduce the memory necessity of the server
					# Insert information into BusRoutes
					index = str(route) + str(direction)
					queryRow = [index, route, json.dumps(stops)]
					if(int(direction) == '0'):
						queryRow.append(False)
					else:
						queryRow.append(True)

					# Ready query for BusRoutes
					queryBusRoutes = "INSERT INTO BusRoutes (RouteDirectionID, RouteID, Stops, Direction) VALUES (%s, %s, %s, %s)"

					# Execute query
					try:
						cursor = conn.cursor()
						cursor.execute(queryBusRoutes, queryRow)
						conn.commit()
						cursor.close()
					except Error as error:
						print("Error while setting values for row of RouteID", route, f"on table BusRoutes -> {error}")
						return False

				else:	# Already have stops from this route, skip until next trip
					for rowStopTimes in linesStopTimes[stopTimesRoutesPosition:]:
						if(rowStopTimes[0] == trip):
							stopTimesRoutesPosition += 1
						else:
							break


				#* tempEntry will be [RouteID, RouteService, stopTimes]
				tempEntry = []
				tempEntry.append(route)
				tempEntry.append(rowTrips[1]) # TODO YOU WERE HERE

				# Go to trip to get starting time
				trip = rowTrips[2]

				#* First check if any of the trips there were out of order is the required one
				hadItSaved = False
				for savedTrip, savedTime in dicTime.items():
					if(savedTrip == trip):	# Match
						tempEntry.append(savedTime)
						hadItSaved = True
						print("Had time of trip", trip, "saved in dictionary")
						del dicTime[f"{trip}"]
						break

				if not hadItSaved:
					timeAdded = False
					#? Order of RouteID is not the same in routes.txt and in stop_times.txt
					for rowStopTimes in linesStopTimes[stopTimesTimePosition:]: 
						if(rowStopTimes[0] == trip):  # Match, get starting time if not already added
							if(len(tempEntry) == 2):
								time = rowStopTimes[1]	
								if(int(time[:2]) >= 24):	# Carris thinks the day has 25 hours sometimes
									timeDiff = 24 - int(time[:2])
									timeShown = 24 + timeDiff
									time = time.replace(str(timeShown), str(timeDiff), 1)
								tempEntry.append(time)
								timeAdded = True
						elif timeAdded:	# Already added, next trip
								break
						else:
							#* Order broke, save next starting time in a dictionary
							# Create new entry in dictionary if it doesn't exist
							if(rowStopTimes[0] not in dicTime.keys()):
								print("Added entry to dicTime for trip", rowStopTimes[0], "->", rowStopTimes[1])
								dicTime[f"{rowStopTimes[0]}"] = rowStopTimes[1]
						
						stopTimesTimePosition += 1

				#? Inserting information like this on the DB will reduce the memory necessity of the server
				# Insert information into BusTrips
				queryRow = [tempEntry[0], tempEntry[1], tempEntry[2]]
				
				# Ready query for BusTrips
				queryBusTrips = "INSERT INTO BusTrips (RouteID, RouteService, StartingTime) VALUES (%s, %s, %s)"

				# Execute query
				try:
					cursor = conn.cursor()
					cursor.execute(queryBusTrips, queryRow)
					conn.commit()
					cursor.close()
				except Error as error:
					print("Error while setting values for row of RouteID", tempEntry[0], ", RouteService", tempEntry[1], "and StartingTime", tempEntry[2], f"on table BusTrips -> {error}")
					return False

			else:
				if matched:	# Already checked everything related to this route
					break
				else:				# Save skipped trip
					skippedTrips.append(rowTrips)
			tripsPosition += 1

	print("Tables BusTrips and BusRoutes set")
	return True

def main():
	# Handle SIGINTfrom user
	signal.signal(signal.SIGINT, handler)
    
	# Download latest data set from Carris
	extractPath = "./carrisMetropolitana/"
	downloadZip(extractPath)

	# Create a connection to the DB
	global conn
	conn = DBConnection()

	# These variables stop useless verification
	clearedBusStops = False
	clearedServiceModes = False
	clearedBusTrips = False
	clearedBusRoutes = False

	# To exit in case of error
	endProgram = False

	# Read command line arguments
	numArgs = len(sys.argv) - 1
	if(numArgs > 1):
		while True:
			if endProgram or argIter == numArgs:
				break
			
			argIter = 1
			if(sys.argv[argIter] == "clear"):
				for arg in range(argIter + 1, numArgs):
					# Clear table BusStops
					if(sys.argv[arg] == "BusStops"):
						if(clearTable("BusStops", conn)):
							clearedBusStops = True
						else:
							break

					# Clear table ServiceModes
					elif(sys.argv[arg] == "ServiceModes"):
						if(clearTable("ServiceModes", conn)):
							clearedServiceModes = True
						else:
							break

					# Clear table BusTrips
					elif(sys.argv[arg] == "BusTrips"):
						if(clearTable("BusTrips", conn)):
							clearedBusTrips = True
						else:
							break

					# Clear table BusRoutes
					elif(sys.argv[arg] == "BusRoutes"):
						if(clearTable("BusRoutes", conn)):
							clearedBusRoutes = True
						else:
							break

					elif(sys.argv[arg] == "set"):
						break

					else:
						print("Unrecognized argument or invalid table ->", sys.argv[arg])
						endProgram = True
						break
			
			elif(sys.argv[argIter] == "set"):
				alreadySet = False	# To not set BusTrips and BusRoutes twice
				for arg in range(argIter + 1, numArgs):
					# Set table BusStops
					if(sys.argv[arg] == "BusStops"):
						if not setBusStops(extractPath, conn, clearedBusStops):
							break

					# Set table ServiceModes
					elif(sys.argv[arg] == "ServiceModes"):
						if not setServiceModes(extractPath, conn, clearedServiceModes):
							break

					# Set table BusRoutes and BusTrips
					elif(sys.argv[arg] == "BusTrips" or sys.argv[arg] == "BusRoutes"):
						if not alreadySet:
							alreadySet = True
							if not setBusTripsAndBusRoutes(extractPath, conn, clearedBusTrips, clearedBusRoutes):
								break

					elif(sys.argv[arg] == "set"):
						break

					else:
						print("Unrecognized argument or invalid table ->", sys.argv[arg])
						endProgram = True
						break

			argIter += 1
	else:
		print("Error. No command line arguments were given.")
		print("Insert \"set <tableName>\" to verify and set table.")
		print("Insert \"clear <tableName>\" to clear table.")
		print("Commands can be chained\t(clear <tableName> <tableName> set <tableName>)")

	# Terminate DB connection
	conn.close()
	print("All done")

if(__name__ == "__main__"):
    main()

