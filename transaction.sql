DELIMITER //



CREATE PROCEDURE MakeTransaction(

    IN p_sender VARCHAR(100),

    IN p_receiver VARCHAR(100),

    IN p_amount FLOAT

)

BEGIN

    DECLARE sender_balance FLOAT DEFAULT 0;

    DECLARE sender_currency BOOL;

    DECLARE receiver_currency BOOL;

    DECLARE receiver_exists INT DEFAULT 0;

    DECLARE sender_exists INT DEFAULT 0;

    DECLARE has_error BOOL DEFAULT FALSE;

    DECLARE error_message VARCHAR(255) DEFAULT '';

    DECLARE converted_amount FLOAT;



    -- Handlers for exceptions and warnings

    DECLARE CONTINUE HANDLER FOR SQLWARNING

    BEGIN

        SET has_error = TRUE;

        GET DIAGNOSTICS CONDITION 1 error_message = MESSAGE_TEXT;

    END;



    DECLARE CONTINUE HANDLER FOR SQLEXCEPTION

    BEGIN

        SET has_error = TRUE;

        GET DIAGNOSTICS CONDITION 1 error_message = MESSAGE_TEXT;

    END;



    -- Set the transaction isolation level to SERIALIZABLE

    SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;

    START TRANSACTION;



    -- Check that the amount is greater than 0

    IF p_amount <= 0 THEN

        SET has_error = TRUE;

        SET error_message = 'Transaction amount must be greater than 0';

    END IF;



    -- Check that both sender and receiver exist

    SELECT COUNT(*), currency INTO sender_exists, sender_currency FROM Balance WHERE username = p_sender;

    SELECT COUNT(*), currency INTO receiver_exists, receiver_currency FROM Balance WHERE username = p_receiver;



    IF sender_exists = 0 THEN

        SET has_error = TRUE;

        SET error_message = 'Sender does not exist';

    END IF;



    IF receiver_exists = 0 THEN

        SET has_error = TRUE;

        SET error_message = 'Receiver does not exist';

    END IF;



    -- Check that sender has sufficient funds and set the conversion amount

    IF NOT has_error THEN

        SELECT balance INTO sender_balance FROM Balance WHERE username = p_sender;

        IF sender_currency = receiver_currency THEN

            SET converted_amount = p_amount;

        ELSEIF sender_currency THEN -- sender has Churricoins, receiver needs Euros

            SET converted_amount = p_amount / 0.96; 

        ELSE -- sender has Euros, receiver needs Churricoins

            SET converted_amount = p_amount * 0.96;

        END IF;



        IF sender_balance < converted_amount THEN

            SET has_error = TRUE;

            SET error_message = 'Insufficient funds';

        END IF;

    END IF;



    -- Perform the transaction if no errors

    IF NOT has_error THEN

        UPDATE Balance SET balance = balance - p_amount WHERE username = p_sender;

        UPDATE Balance SET balance = balance + converted_amount WHERE username = p_receiver;



        -- Record the transaction

        INSERT INTO Transactions (sender, receiver, amount, currency_kind) VALUES (p_sender, p_receiver, p_amount, sender_currency);



        -- Commit the transaction

        COMMIT;

    ELSE

        -- Rollback the transaction in case of an error

        ROLLBACK;



        -- Raise an error with the captured message

        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = error_message;

    END IF;

END //



DELIMITER ;
