<html>
  <body>
    <?php
    // Get key values for database connection
    $hostname = getenv("HOSTNAME");
    $dbname = getenv("dbnameSecure");
    $username = getenv("usernameSecure");
    $password = getenv("passwordSecure");
    $port = getenv("portSecure");
    
    // GET variables
    $latitude = $_GET["Latitude"];
    $longitude = $_GET["Longitude"];
    $altitude = $_GET["Altitude"];
    $date = $_GET["Date"];
    $UPLYID = $_GET["UPLYID"];
    $openingUTCTime = $_GET["OpeningUTCTime"];
    $closingUTCTime = $_GET["ClosingUTCTime"];
    $openedTime = $_GET["OpenedTime"];
    $diffPeople = $_GET["DiffPeople"];
    $diffWeight = $_GET["DiffWeight"];
    $capacity = $_GET["Capacity"];
            
    // Create connection and insert into table
    $conn = new mysqli($hostname, $username, $password, $dbname, $port);
    if ($conn->connect_error) 
      die("Connection failed: " . $conn->connect_error);

    $stmt = $conn->prepare("INSERT INTO Openings (OpeningUTCTime, ClosingUTCTime, OpenedTime, Latitude, Longitude, Altitude, Date, UPLYID, DiffPeople, DiffWeight, Capacity) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    $stmt->bind_param("sssdddsdddd", $openingUTCTime, $closingUTCTime, $openedTime, $latitude, $longitude, $altitude, $date, $UPLYID, $diffPeople, $diffWeight, $capacity);
    
    $query = $stmt->execute();

    // Check for erros
    if($query === TRUE)
      echo "Change made successfully";
    else
      echo "An error ocurred: ". $conn->error;
        
    // Close the connection
    $stmt->close();
    $conn->close();
    ?> 
  </body>
</html>