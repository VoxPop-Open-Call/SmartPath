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
    $entryLatitude = $_GET["EntryLatitude"];
    $entryLongitude = $_GET["EntryLongitude"];
    $exitLatitude = $_GET["ExitLatitude"];
    $exitLongitude = $_GET["ExitLongitude"];
    $entryTime = $_GET["EntryTime"];
    $exitTime = $_GET["ExitTime"];
    $weight = $_GET["Weight"];
    $UPLYID = $_GET["UPLYID"];
    $date = $_GET["Date"];
            
    // Create connection and insert into table
    $conn = new mysqli($hostname, $username, $password, $dbname, $port);
    if ($conn->connect_error) 
      die("Connection failed: " . $conn->connect_error);

    $stmt = $conn->prepare("INSERT INTO Passengers (EntryLatitude, EntryLongitude, ExitLatitude, ExitLongitude, EntryTime, ExitTime, Date, Weight, UPLYID) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);");
    $stmt->bind_param("ddddsssdd", $entryLatitude, $entryLongitude, $exitLatitude, $exitLongitude, $entryTime, $exitTime, $weight, $UPLYID, $date);
    
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