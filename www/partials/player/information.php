<div class="col-sm-8" id="player-information">
    <h1 class="player_name"></h1>

    <div id="suicide-alert" class="alert alert-dismissible alert-danger hidden">
        <button type="button" class="close" data-dismiss="alert">×</button>
        <strong>Suicide Notice:</strong> <span class="player_name"></span> committed suicide at <span class="player_suicide_time"></span>
    </div>

    <div class="row">
        <div class="col-md-6">
            <?php require_once 'partials/identification.php'; ?>

            <?php require_once 'partials/logins.php'; ?>
        </div>
        <div class="col-md-6">
            <?php require_once 'partials/stats.php'; ?>

            <?php require_once 'partials/deaths.php'; ?>
        </div> <!-- col-md-6 -->
    </div> <!-- row -->
</div> <!-- col-md-9 -->